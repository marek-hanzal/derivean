#include "atmosphere/fn/SphereOperators.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

double vertexValue(const SphereGrid& grid, const SphereField& field, int vertex) {
    const SphereVertex& v = grid.vertex[size_t(vertex)];
    double sum = 0;
    for (int k = 0; k < v.count; ++k) sum += v.weight[size_t(k)] * field[size_t(v.cells[size_t(k)])];
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

Stencil stencilOf(const SphereGrid& grid, const SphereEdge& edge) {
    Stencil stencil;
    addTo(stencil, edge.left, -edge.across);
    addTo(stencil, edge.right, edge.across);
    const SphereVertex& to = grid.vertex[size_t(edge.to)];
    const SphereVertex& from = grid.vertex[size_t(edge.from)];
    for (int k = 0; k < to.count; ++k) addTo(stencil, to.cells[size_t(k)], edge.along * to.weight[size_t(k)]);
    for (int k = 0; k < from.count; ++k) addTo(stencil, from.cells[size_t(k)], edge.along * -from.weight[size_t(k)]);
    return stencil;
}

struct Entry {
    int column;
    double value;
};

} // namespace

void vertexValues(const SphereGrid& grid, const SphereField& field, SphereField& result) {
    result.resize(grid.vertex.size());
    for (size_t v = 0; v < result.size(); ++v) result[v] = vertexValue(grid, field, int(v));
}

SphereField vertexValues(const SphereGrid& grid, const SphereField& field) {
    SphereField values;
    vertexValues(grid, field, values);
    return values;
}

double dotOf(const SphereField& a, const SphereField& b) {
    double sum = 0;
    for (size_t k = 0; k < a.size(); ++k) sum += a[k] * b[k];
    return sum;
}

EdgeVectors gradientOf(const SphereGrid& grid, const SphereField& field) {
    const SphereField at = vertexValues(grid, field);
    EdgeVectors gradient(grid.edge.size());
    for (size_t e = 0; e < gradient.size(); ++e) {
        const SphereEdge& edge = grid.edge[e];
        const double across = field[size_t(edge.right)] - field[size_t(edge.left)];
        const double along = at[size_t(edge.to)] - at[size_t(edge.from)];
        gradient[e] = edge.across * across + edge.along * along;
    }
    return gradient;
}

EdgeField normalFluxOf(const SphereGrid& grid, const EdgeVectors& vectors) {
    EdgeField flux(grid.edge.size());
    for (size_t e = 0; e < flux.size(); ++e) flux[e] = dot(vectors[e], grid.edge[e].normal) * grid.edge[e].length;
    return flux;
}

SphereField divergenceOf(const SphereGrid& grid, const EdgeField& flux) {
    SphereField result(size_t(grid.cells), 0.0);
    for (size_t e = 0; e < flux.size(); ++e) {
        result[size_t(grid.edge[e].left)] += flux[e];
        result[size_t(grid.edge[e].right)] -= flux[e];
    }
    for (int c = 0; c < grid.cells; ++c) result[size_t(c)] /= grid.area[size_t(c)];
    return result;
}

SphereField curlOf(const SphereGrid& grid, const EdgeVectors& vectors) {
    EdgeField circulation(grid.edge.size());
    for (size_t e = 0; e < circulation.size(); ++e) circulation[e] = dot(vectors[e], grid.edge[e].chord);
    return divergenceOf(grid, circulation);
}

EdgeField streamFluxOf(const SphereGrid& grid, const SphereField& stream) {
    EdgeField flux(grid.edge.size());
    for (size_t e = 0; e < flux.size(); ++e)
        flux[e] = vertexValue(grid, stream, grid.edge[e].from) - vertexValue(grid, stream, grid.edge[e].to);
    return flux;
}

SphereMatrix makeLaplacian(const SphereGrid& grid) {
    // Rows gather their entries edge by edge, in edge order for every row, so K(p, q) and K(q, p)
    // add the same products in the same order and the matrix is symmetric bit for bit.
    std::vector<std::vector<Entry>> rows(size_t(grid.cells));
    for (auto& row : rows) row.reserve(25);
    for (const SphereEdge& edge : grid.edge) {
        const Stencil stencil = stencilOf(grid, edge);
        for (int p = 0; p < stencil.count; ++p) {
            std::vector<Entry>& row = rows[size_t(stencil.cell[p])];
            for (int q = 0; q < stencil.count; ++q) {
                const double value = edge.diamond * dot(stencil.weight[p], stencil.weight[q]);
                auto found = std::find_if(row.begin(), row.end(), [&](const Entry& e) { return e.column == stencil.cell[q]; });
                if (found == row.end()) row.push_back({stencil.cell[q], value});
                else found->value += value;
            }
        }
    }
    SphereMatrix matrix;
    matrix.start.reserve(rows.size() + 1);
    matrix.diagonal.resize(rows.size());
    matrix.start.push_back(0);
    for (size_t r = 0; r < rows.size(); ++r) {
        std::sort(rows[r].begin(), rows[r].end(), [](const Entry& a, const Entry& b) { return a.column < b.column; });
        for (const Entry& entry : rows[r]) {
            matrix.column.push_back(entry.column);
            matrix.value.push_back(entry.value);
            if (entry.column == int(r)) matrix.diagonal[r] = entry.value;
        }
        matrix.start.push_back(int(matrix.column.size()));
    }
    return matrix;
}

void applyMatrix(const SphereMatrix& matrix, const SphereField& field, SphereField& result) {
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

SphereField laplacianOf(const SphereGrid& grid, const SphereMatrix& stiffness, const SphereField& field) {
    SphereField result;
    applyMatrix(stiffness, field, result);
    for (int c = 0; c < grid.cells; ++c) result[size_t(c)] = -result[size_t(c)] / grid.area[size_t(c)];
    return result;
}

double areaMean(const SphereGrid& grid, const SphereField& field) {
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
