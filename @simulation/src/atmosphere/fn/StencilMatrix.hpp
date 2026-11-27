#pragma once

#include "atmosphere/type/SphereMatrix.hpp"
#include "atmosphere/type/StencilMatrix.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace derivean::sim {

// Gives the six faces of an assembled Laplacian the interior of face 0. The cube's rotations carry
// face 0 with its cell numbering onto every other face, so the interiors agree to rounding (6.5e-14
// of the largest entry at level 8); the rounding differs because each face's centres are computed
// from its own coordinates. Face 0's rows are the canonical ones: they replace the other faces'
// interior rows and diagonals, and every seam row's entry that couples to an interior row takes that
// row's value, so K stays symmetric bit for bit. Returns the largest difference found, relative to
// the largest entry; above `limit` the matrix is left as it was. Rows not on the 21-point pattern
// leave it as it was too (and return 0).
double shareFaceInteriors(SphereMatrix& matrix, int side, double limit = 1e-10);

// Splits an assembled Laplacian on a grid with `side` cells along a face edge. Should an interior row
// not have the 21 expected columns, every row stays a compressed row.
StencilMatrix makeStencilMatrix(const SphereMatrix& matrix, int side);

// Calls row(r, (K · field)[r]) for every row r in ascending order, so callers can fuse their own
// loop with the product; each sum is the one applyMatrix gives, bit for bit.
template <class Row> void forEachProduct(const StencilMatrix& matrix, const double* x, Row&& row) {
    constexpr int Margin = 2, Chunk = 64, Entries = 21;
    const int side = matrix.side, run = side - 2 * Margin;
    const size_t seams = matrix.seamRow.size();
    size_t k = 0;
    auto seamsBefore = [&](int end) {
        for (; k < seams && matrix.seamRow[k] < end; ++k) {
            double sum = 0;
            for (int e = matrix.start[k]; e < matrix.start[k + 1]; ++e) sum += matrix.value[size_t(e)] * x[matrix.column[size_t(e)]];
            row(size_t(matrix.seamRow[k]), sum);
        }
    };
    const double* faceValues = matrix.inner.data();
    for (int face = 0; face < 6; ++face) {
        const double* v = matrix.shared ? matrix.inner.data() : faceValues;
        for (int j = 0; j < side; ++j) {
            const int line = (face * side + j) * side;
            if (matrix.interior == 0 || j < Margin || j >= side - Margin) {
                seamsBefore(line + side);
                continue;
            }
            seamsBefore(line + Margin);
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0), first = line + Margin + i0;
                int i = 0;
                // Eight rows at a time with their sums in registers; each sum adds its 21 products in
                // column order, as the compressed row does.
                for (; i + 8 <= count; i += 8) {
                    double sum[8] = {};
                    for (int e = 0; e < Entries; ++e) {
                        const double* value = v + e * count + i;
                        const double* in = x + first + i + matrix.offset[size_t(e)];
                        for (int u = 0; u < 8; ++u) sum[u] += value[u] * in[u];
                    }
                    for (int u = 0; u < 8; ++u) row(size_t(first + i + u), sum[u]);
                }
                for (; i < count; ++i) {
                    double sum = 0;
                    for (int e = 0; e < Entries; ++e) sum += v[e * count + i] * x[first + i + matrix.offset[size_t(e)]];
                    row(size_t(first + i), sum);
                }
                v += Entries * count;
            }
            seamsBefore(line + side);
        }
        faceValues = v;
    }
}

// result = K · field, the same bit for bit as applyMatrix on the SphereMatrix it came from.
void applyMatrix(const StencilMatrix& matrix, const std::vector<double>& field, std::vector<double>& result);

} // namespace derivean::sim
