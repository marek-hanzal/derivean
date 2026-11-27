#include "atmosphere/fn/SphereOperators.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>

namespace derivean::sim {
namespace {

double vertexValue(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field, int vertex) {
    const SphereVertex& v = grid.vertex[size_t(vertex)];
    const std::array<double, 4>& weight = stencil.weight[size_t(vertex)];
    double sum = 0;
    for (int k = 0; k < v.count; ++k) sum += weight[size_t(k)] * field[size_t(v.cells[size_t(k)])];
    return sum;
}

// One edge's gradient as weights on the cells it reads: −across on left, +across on right, ±along
// shared by the cells of the two vertices. Left and right meet both vertices, so their entries merge.
struct Stencil {
    int count{};
    int cell[10]{};
    Vec3 weight[10];
};

void addTo(Stencil& stencil, int cell, Vec3 weight) {
    for (int k = 0; k < stencil.count; ++k)
        if (stencil.cell[k] == cell) {
            stencil.weight[k] = stencil.weight[k] + weight;
            return;
        }
    stencil.cell[stencil.count] = cell;
    stencil.weight[stencil.count++] = weight;
}

Stencil stencilOf(const SphereGrid& grid, const SphereStencil& tables, size_t e) {
    const SphereEdge& edge = grid.edge[e];
    const EdgeStencil& metric = tables.edge[e];
    Stencil stencil;
    addTo(stencil, edge.left, -metric.across);
    addTo(stencil, edge.right, metric.across);
    const SphereVertex& to = grid.vertex[size_t(edge.to)];
    const SphereVertex& from = grid.vertex[size_t(edge.from)];
    const std::array<double, 4>& toWeight = tables.weight[size_t(edge.to)];
    const std::array<double, 4>& fromWeight = tables.weight[size_t(edge.from)];
    for (int k = 0; k < to.count; ++k) addTo(stencil, to.cells[size_t(k)], metric.along * toWeight[size_t(k)]);
    for (int k = 0; k < from.count; ++k) addTo(stencil, from.cells[size_t(k)], metric.along * -fromWeight[size_t(k)]);
    return stencil;
}

} // namespace

void vertexValues(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field, CellField& result) {
    assert(stencil.weight.size() == grid.vertex.size());
    result.resize(grid.vertex.size());
    for (size_t v = 0; v < result.size(); ++v) result[v] = vertexValue(grid, stencil, field, int(v));
}

CellField vertexValues(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field) {
    CellField values;
    vertexValues(grid, stencil, field, values);
    return values;
}

double dotOf(const CellField& a, const CellField& b) {
    double sum = 0;
    for (size_t k = 0; k < a.size(); ++k) sum += a[k] * b[k];
    return sum;
}

EdgeVectors gradientOf(const SphereGrid& grid, const SphereStencil& stencil, const CellField& field) {
    const CellField at = vertexValues(grid, stencil, field);
    assert(stencil.edge.size() == grid.edge.size());
    EdgeVectors gradient(grid.edge.size());
    for (size_t e = 0; e < gradient.size(); ++e) {
        const SphereEdge& edge = grid.edge[e];
        const double across = field[size_t(edge.right)] - field[size_t(edge.left)];
        const double along = at[size_t(edge.to)] - at[size_t(edge.from)];
        gradient[e] = stencil.edge[e].across * across + stencil.edge[e].along * along;
    }
    return gradient;
}

EdgeField normalFluxOf(const SphereGrid& grid, const EdgeVectors& vectors) {
    EdgeField flux(grid.edge.size());
    for (size_t e = 0; e < flux.size(); ++e) flux[e] = dot(vectors[e], grid.edge[e].normal) * grid.edge[e].length;
    return flux;
}

CellField divergenceOf(const SphereGrid& grid, const EdgeField& flux) {
    CellField result(size_t(grid.cells), 0.0);
    for (size_t e = 0; e < flux.size(); ++e) {
        result[size_t(grid.edge[e].left)] += flux[e];
        result[size_t(grid.edge[e].right)] -= flux[e];
    }
    for (int c = 0; c < grid.cells; ++c) result[size_t(c)] /= grid.area[size_t(c)];
    return result;
}

CellField curlOf(const SphereGrid& grid, const SphereStencil& stencil, const EdgeVectors& vectors) {
    assert(stencil.edge.size() == grid.edge.size());
    EdgeField circulation(grid.edge.size());
    for (size_t e = 0; e < circulation.size(); ++e) circulation[e] = dot(vectors[e], stencil.edge[e].chord);
    return divergenceOf(grid, circulation);
}

EdgeField streamFluxOf(const SphereGrid& grid, const SphereStencil& stencil, const CellField& stream) {
    assert(stencil.weight.size() == grid.vertex.size());
    EdgeField flux(grid.edge.size());
    for (size_t e = 0; e < flux.size(); ++e)
        flux[e] = vertexValue(grid, stencil, stream, grid.edge[e].from) - vertexValue(grid, stencil, stream, grid.edge[e].to);
    return flux;
}

SphereMatrix makeLaplacian(const SphereGrid& grid, const SphereStencil& tables) {
    // Rows gather their entries edge by edge, in edge order for every row, so K(p, q) and K(q, p)
    // add the same products in the same order and the matrix is symmetric bit for bit. The rows
    // share one flat buffer of `capacity` slots each (rows have 21 entries, 16 on level 1), found
    // through a per-row hash of twice that many places; a row that outgrows it restarts the assembly
    // with twice the room. A row reads at most 12 edges of at most 10 cells, so 128 slots (bits = 7)
    // always fit and a slot number + 1 fits the hash's byte.
    assert(tables.edge.size() == grid.edge.size() && tables.weight.size() == grid.vertex.size());
    const size_t cells = size_t(grid.cells);
    std::vector<int> count(cells);
    for (int bits = 5; bits <= 7; ++bits) {
        const size_t capacity = size_t(1) << bits, places = capacity * 2;
        const auto columns = std::make_unique_for_overwrite<int[]>(cells * capacity);
        const auto values = std::make_unique_for_overwrite<double[]>(cells * capacity);
        std::vector<uint8_t> hash(cells * places, uint8_t(0));   // slot + 1, 0 is empty
        std::fill(count.begin(), count.end(), 0);
        bool fits = true;
        for (size_t e = 0; e < grid.edge.size() && fits; ++e) {
            const Stencil stencil = stencilOf(grid, tables, e);
            const double diamond = tables.edge[e].diamond;
            for (int p = 0; p < stencil.count && fits; ++p) {
                const size_t row = size_t(stencil.cell[p]);
                int* column = &columns[row * capacity];
                double* value = &values[row * capacity];
                uint8_t* place = &hash[row * places];
                int& used = count[row];
                for (int q = 0; q < stencil.count; ++q) {
                    const double add = diamond * dot(stencil.weight[p], stencil.weight[q]);
                    const int cell = stencil.cell[q];
                    size_t h = (uint32_t(cell) * 2654435761u) >> (31 - bits);
                    while (place[h] != 0 && column[place[h] - 1] != cell) h = (h + 1) & (places - 1);
                    if (place[h] != 0) value[place[h] - 1] += add;
                    else if (size_t(used) < capacity) {
                        place[h] = uint8_t(used + 1);
                        column[used] = cell;
                        value[used++] = add;
                    } else {
                        fits = false;
                        break;
                    }
                }
            }
        }
        if (!fits) continue;
        SphereMatrix matrix;
        matrix.start.resize(cells + 1);
        matrix.diagonal.resize(cells);
        for (size_t r = 0; r < cells; ++r) matrix.start[r + 1] = matrix.start[r] + count[r];
        matrix.column.resize(size_t(matrix.start[cells]));
        matrix.value.resize(size_t(matrix.start[cells]));
        std::vector<int> order(capacity);
        for (size_t r = 0; r < cells; ++r) {
            const int* column = &columns[r * capacity];
            const double* value = &values[r * capacity];
            const int used = count[r];
            for (int k = 0; k < used; ++k) order[size_t(k)] = k;
            std::sort(order.begin(), order.begin() + used, [&](int a, int b) { return column[a] < column[b]; });
            for (int k = 0; k < used; ++k) {
                const size_t at = size_t(matrix.start[r] + k);
                matrix.column[at] = column[order[size_t(k)]];
                matrix.value[at] = value[order[size_t(k)]];
                if (matrix.column[at] == int(r)) matrix.diagonal[r] = matrix.value[at];
            }
        }
        return matrix;
    }
    std::abort();   // unreachable: 128 slots hold any row (above)
}

void applyMatrix(const SphereMatrix& matrix, const CellField& field, CellField& result) {
    const size_t rows = matrix.diagonal.size();
    result.resize(rows);
    const int* column = matrix.column.data();
    const double* value = matrix.value.data();
    for (size_t r = 0; r < rows; ++r) {
        double sum = 0;
        for (int k = matrix.start[r]; k < matrix.start[r + 1]; ++k) sum += value[k] * field[size_t(column[k])];
        result[r] = sum;
    }
}

CellField laplacianOf(const SphereGrid& grid, const SphereMatrix& stiffness, const CellField& field) {
    CellField result;
    applyMatrix(stiffness, field, result);
    for (int c = 0; c < grid.cells; ++c) result[size_t(c)] = -result[size_t(c)] / grid.area[size_t(c)];
    return result;
}

double areaMean(const SphereGrid& grid, const CellField& field) {
    // Neumaier (1974) summation: the sum spans 5e14 m² from cells of 1e9 m².
    double sum = 0, carry = 0, area = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double term = grid.area[size_t(c)] * field[size_t(c)], next = sum + term;
        carry += std::abs(sum) >= std::abs(term) ? (sum - next) + term : (term - next) + sum;
        sum = next;
        area += grid.area[size_t(c)];
    }
    return (sum + carry) / area;
}

} // namespace derivean::sim
