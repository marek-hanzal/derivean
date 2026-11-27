#pragma once

#include "atmosphere/type/SphereMatrix.hpp"
#include "atmosphere/type/StencilMatrix.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace derivean::sim {

// Gives the six faces of an assembled Laplacian the interior of face 0. The cube's rotations carry
// face 0 with its cell numbering onto every other face, so the interiors agree to rounding (6.5e-14
// of the largest entry at level 8); the rounding differs because each face's centres are computed
// from its own coordinates. Face 0's rows are the canonical ones: they replace the other faces'
// interior rows and diagonals, and every seam row's entry that couples to an interior row takes that
// row's value, so K stays symmetric bit for bit (K must be structurally symmetric, as makeLaplacian
// builds it). Returns the largest difference found, relative to the largest entry; above `limit` the
// matrix is left as it was. Without interior rows on the 21-point pattern it is left as it was too
// and the result is negative.
double shareFaceInteriors(SphereMatrix& matrix, int side, double limit = 1e-10);

// Splits an assembled Laplacian on a grid with `side` cells along a face edge. Should an interior row
// not have the 21 expected columns, every row stays a compressed row.
StencilMatrix makeStencilMatrix(const SphereMatrix& matrix, int side);

// Seam row k of K · x, its products added in column order as the compressed row does. These sums
// must stay fused, to equal the NEON kernel's, and so stay scalar: vectorised in order, clang on arm64
// would round each product before adding it (docs/windows.md#determinism).
template <class Real> Real seamProduct(const StencilMatrixOf<Real>& matrix, size_t k, const Real* x) {
    Real sum = 0;
#pragma clang loop vectorize(disable)
    for (int e = matrix.start[k]; e < matrix.start[k + 1]; ++e) sum += matrix.value[size_t(e)] * x[matrix.column[size_t(e)]];
    return sum;
}

// Rows from `from` on of one block of `count` interior rows starting at row `first`, whose values `v`
// are stored entry by entry: eight rows at a time with their sums in registers, then one by one. Each
// sum adds its 21 products in column order, as the compressed row does.
template <class Real, class Row>
void blockProduct(const StencilMatrixOf<Real>& matrix, const Real* v, int count, int first, const Real* x, int from, Row&& row) {
    int i = from;
    for (; i + 8 <= count; i += 8) {
        Real sum[8] = {};
        for (int e = 0; e < StencilMatrix::Entries; ++e) {
            const Real* value = v + e * count + i;
            const Real* in = x + first + i + matrix.offset[size_t(e)];
            for (int u = 0; u < 8; ++u) sum[u] += value[u] * in[u];
        }
        for (int u = 0; u < 8; ++u) row(size_t(first + i + u), sum[u]);
    }
    for (; i < count; ++i) {
        Real sum = 0;
#pragma clang loop vectorize(disable)   // scalar and fused, as in seamProduct
        for (int e = 0; e < StencilMatrix::Entries; ++e) sum += v[e * count + i] * x[first + i + matrix.offset[size_t(e)]];
        row(size_t(first + i), sum);
    }
}

// Calls row(r, (K · field)[r]) for every row r in ascending order, so callers can fuse their own
// loop with the product; each sum is the one applyMatrix gives, bit for bit.
template <class Real, class Row> void forEachProduct(const StencilMatrixOf<Real>& matrix, const Real* x, Row&& row) {
    constexpr int Margin = StencilMatrix::Margin, Chunk = StencilMatrix::Chunk, Entries = StencilMatrix::Entries;
    const int side = matrix.side, run = side - 2 * Margin;
    const size_t seams = matrix.seamRow.size();
    size_t k = 0;
    auto seamsBefore = [&](int end) {
        for (; k < seams && matrix.seamRow[k] < end; ++k) row(size_t(matrix.seamRow[k]), seamProduct(matrix, k, x));
    };
    const Real* faceValues = matrix.inner.data();
    for (int face = 0; face < 6; ++face) {
        const Real* v = matrix.shared ? matrix.inner.data() : faceValues;
        for (int j = 0; j < side; ++j) {
            const int line = (face * side + j) * side;
            if (matrix.interior == 0 || j < Margin || j >= side - Margin) {
                seamsBefore(line + side);
                continue;
            }
            seamsBefore(line + Margin);
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0);
                blockProduct(matrix, v, count, line + Margin + i0, x, 0, row);
                v += Entries * count;
            }
            seamsBefore(line + side);
        }
        faceValues = v;
    }
}

// Calls row(r, Σ_j |K_rj|) for every row r in ascending order, each sum's terms in column order as
// forEachProduct adds them: bit for bit forEachProduct of |K| with a field of ones (a product by one
// is exact, fused or not), without the copy of K that takes (Neon DREV-NEON-065).
template <class Real, class Row> void forEachMagnitudeSum(const StencilMatrixOf<Real>& matrix, Row&& row) {
    constexpr int Margin = StencilMatrix::Margin, Chunk = StencilMatrix::Chunk, Entries = StencilMatrix::Entries;
    const int side = matrix.side, run = side - 2 * Margin;
    const size_t seams = matrix.seamRow.size();
    size_t k = 0;
    auto seamsBefore = [&](int end) {
        for (; k < seams && matrix.seamRow[k] < end; ++k) {
            Real sum = 0;
            for (int e = matrix.start[k]; e < matrix.start[k + 1]; ++e) sum += std::abs(matrix.value[size_t(e)]);
            row(size_t(matrix.seamRow[k]), sum);
        }
    };
    const Real* faceValues = matrix.inner.data();
    for (int face = 0; face < 6; ++face) {
        const Real* v = matrix.shared ? matrix.inner.data() : faceValues;
        for (int j = 0; j < side; ++j) {
            const int line = (face * side + j) * side;
            if (matrix.interior == 0 || j < Margin || j >= side - Margin) {
                seamsBefore(line + side);
                continue;
            }
            seamsBefore(line + Margin);
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0);
                for (int i = 0; i < count; ++i) {
                    Real sum = 0;
                    for (int e = 0; e < Entries; ++e) sum += std::abs(v[e * count + i]);
                    row(size_t(line + Margin + i0 + i), sum);
                }
                v += Entries * count;
            }
            seamsBefore(line + side);
        }
        faceValues = v;
    }
}

// result = K · field, the same bit for bit as applyMatrix on the SphereMatrix it came from and as
// forEachProduct; field and result must be different vectors. A shared matrix on arm64 takes a NEON
// kernel whose fused multiply-adds equal the scalar sums only while clang fuses `sum += a * b`
// (-ffp-contract=on, clang's default for C++; docs/credits/libraries.md, platform documentation).
void applyMatrix(const StencilMatrix& matrix, const std::vector<double>& field, std::vector<double>& result);

// The same in float32 on the narrowed copy: each row's sum in the same order, so the product is the
// same bit for bit on arm64 (NEON) and x86-64 (AVX2 and FMA), and the same as forEachProduct's.
void applyMatrix(const StencilMatrixOf<float>& matrix, const std::vector<float>& field, std::vector<float>& result);

// The matrix with its values rounded to float32, for the V-cycle.
StencilMatrixOf<float> narrowed(const StencilMatrix& matrix);

} // namespace derivean::sim
