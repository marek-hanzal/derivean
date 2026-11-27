#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cstdlib>

namespace derivean::sim {
namespace {

// Interior rows are visited face by face, grid line by grid line, i from 2 to side − 3, which is
// also their order in `inner`.
constexpr int Margin = 2;
constexpr int Chunk = 64;   // rows of a grid line whose values are stored together

bool insideFace(int side, int row) {
    const int rest = row % (side * side), i = rest % side, j = rest / side;
    return i >= Margin && i < side - Margin && j >= Margin && j < side - Margin;
}

} // namespace

StencilMatrix makeStencilMatrix(const SphereMatrix& matrix, int side) {
    StencilMatrix split;
    split.side = side;
    split.diagonal = matrix.diagonal;
    const int rows = int(matrix.diagonal.size());
    int k = 0;
    for (int dj = -Margin; dj <= Margin; ++dj)
        for (int di = -Margin; di <= Margin; ++di)
            if (std::abs(di) + std::abs(dj) < 2 * Margin) split.offset[size_t(k++)] = dj * side + di;
    bool regular = side >= 2 * Margin + 1;
    for (int r = 0; r < rows && regular; ++r) {
        if (!insideFace(side, r)) continue;
        const int first = matrix.start[size_t(r)], count = matrix.start[size_t(r) + 1] - first;
        regular = count == int(split.offset.size());
        for (int e = 0; e < count && regular; ++e) regular = matrix.column[size_t(first + e)] - r == split.offset[size_t(e)];
    }
    split.start.push_back(0);
    for (int r = 0; r < rows; ++r) {
        if (regular && insideFace(side, r)) {
            ++split.interior;
            continue;
        }
        split.seamRow.push_back(r);
        for (int e = matrix.start[size_t(r)]; e < matrix.start[size_t(r) + 1]; ++e) {
            split.column.push_back(matrix.column[size_t(e)]);
            split.value.push_back(matrix.value[size_t(e)]);
        }
        split.start.push_back(int(split.column.size()));
    }
    // Values by block of up to Chunk rows of one grid line, entry by entry within the block, so the
    // product reads them in one sequential pass.
    split.inner.reserve(split.offset.size() * size_t(split.interior));
    const int run = side - 2 * Margin;
    for (int face = 0; face < 6 && regular; ++face)
        for (int j = Margin; j < side - Margin; ++j)
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0), row = (face * side + j) * side + Margin + i0;
                for (size_t e = 0; e < split.offset.size(); ++e)
                    for (int i = 0; i < count; ++i) split.inner.push_back(matrix.value[size_t(matrix.start[size_t(row + i)]) + e]);
            }
    return split;
}

void applyMatrix(const StencilMatrix& matrix, const std::vector<double>& field, std::vector<double>& result) {
    result.resize(matrix.diagonal.size());
    const double* x = field.data();
    double* y = result.data();
    for (size_t k = 0; k < matrix.seamRow.size(); ++k) {
        double sum = 0;
        for (int e = matrix.start[k]; e < matrix.start[k + 1]; ++e) sum += matrix.value[size_t(e)] * x[matrix.column[size_t(e)]];
        y[matrix.seamRow[k]] = sum;
    }
    if (matrix.interior == 0) return;
    const int side = matrix.side, run = side - 2 * Margin;
    constexpr int Entries = 21;
    const double* v = matrix.inner.data();
    for (int face = 0; face < 6; ++face)
        for (int j = Margin; j < side - Margin; ++j)
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0), row = (face * side + j) * side + Margin + i0;
                int i = 0;
                // Eight rows at a time with their sums in registers; each sum adds its 21 products in
                // column order, as the compressed row does.
                for (; i + 8 <= count; i += 8) {
                    double sum[8] = {};
                    for (int e = 0; e < Entries; ++e) {
                        const double* value = v + e * count + i;
                        const double* in = x + row + i + matrix.offset[size_t(e)];
                        for (int u = 0; u < 8; ++u) sum[u] += value[u] * in[u];
                    }
                    for (int u = 0; u < 8; ++u) y[row + i + u] = sum[u];
                }
                for (; i < count; ++i) {
                    double sum = 0;
                    for (int e = 0; e < Entries; ++e) sum += v[e * count + i] * x[row + i + matrix.offset[size_t(e)]];
                    y[row + i] = sum;
                }
                v += Entries * count;
            }
}

} // namespace derivean::sim
