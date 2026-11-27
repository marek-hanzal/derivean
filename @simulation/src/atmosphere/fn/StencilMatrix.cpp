#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

namespace derivean::sim {
namespace {

// `inner` holds the interior rows face by face, grid line by grid line, i from 2 to side − 3.
constexpr int Margin = StencilMatrix::Margin, Chunk = StencilMatrix::Chunk, Entries = StencilMatrix::Entries;

bool insideFace(int side, int row) {
    const int rest = row % (side * side), i = rest % side, j = rest / side;
    return i >= Margin && i < side - Margin && j >= Margin && j < side - Margin;
}

// Grid line dj outer, neighbour di inner; applyEightInteriorRowsNeon adds its taps in this order.
std::array<int, Entries> offsetsFor(int side) {
    std::array<int, Entries> offset{};
    int k = 0;
    for (int dj = -Margin; dj <= Margin; ++dj)
        for (int di = -Margin; di <= Margin; ++di)
            if (std::abs(di) + std::abs(dj) < 2 * Margin) offset[size_t(k++)] = dj * side + di;
    return offset;
}

// Every interior row has the 21 columns row + offset, in that order.
bool regularOf(const SphereMatrix& matrix, int side, const std::array<int, Entries>& offset) {
    if (side < 2 * Margin + 1) return false;
    const int rows = int(matrix.diagonal.size());
    for (int r = 0; r < rows; ++r) {
        if (!insideFace(side, r)) continue;
        const int first = matrix.start[size_t(r)];
        if (matrix.start[size_t(r) + 1] - first != Entries) return false;
        for (int e = 0; e < Entries; ++e)
            if (matrix.column[size_t(first + e)] - r != offset[size_t(e)]) return false;
    }
    return true;
}

#if defined(__aarch64__)
// Eight consecutive interior rows of one block. Each of the five grid lines dj = −2..2 is loaded
// once, and the neighbours di are formed in registers by vextq_f64 instead of reloading x per
// entry. The taps run in offsetsFor's order (dj outer, di inner), so every sum adds its 21 products
// in column order as the compressed row does; vfmaq_f64 fuses each product with its sum, which is
// what clang makes of `sum += value * x` under -ffp-contract=on (Arm Neon Intrinsics Reference and
// the Clang manual, docs/credits/libraries.md, platform documentation).
void applyEightInteriorRowsNeon(const double* __restrict blockValues, int blockCount,
                                const double* __restrict centerInput, int side,
                                double* __restrict outputPointer) {
    float64x2_t sum0 = vdupq_n_f64(0.0), sum1 = sum0, sum2 = sum0, sum3 = sum0;
    const double* weightPointer = blockValues;

    auto accumulateTap = [&](float64x2_t lane0, float64x2_t lane1, float64x2_t lane2, float64x2_t lane3) {
        sum0 = vfmaq_f64(sum0, vld1q_f64(weightPointer + 0), lane0);
        sum1 = vfmaq_f64(sum1, vld1q_f64(weightPointer + 2), lane1);
        sum2 = vfmaq_f64(sum2, vld1q_f64(weightPointer + 4), lane2);
        sum3 = vfmaq_f64(sum3, vld1q_f64(weightPointer + 6), lane3);
        weightPointer += blockCount;
    };

    auto accumulateThreeTaps = [&](const double* rowPointer) {
        const float64x2_t xM1_0 = vld1q_f64(rowPointer - 1), x1_2 = vld1q_f64(rowPointer + 1);
        const float64x2_t x3_4  = vld1q_f64(rowPointer + 3), x5_6 = vld1q_f64(rowPointer + 5);
        const float64x2_t x7_8  = vld1q_f64(rowPointer + 7);
        accumulateTap(xM1_0, x1_2, x3_4, x5_6);
        accumulateTap(vextq_f64(xM1_0, x1_2, 1), vextq_f64(x1_2, x3_4, 1),
                      vextq_f64(x3_4, x5_6, 1),  vextq_f64(x5_6, x7_8, 1));
        accumulateTap(x1_2, x3_4, x5_6, x7_8);
    };

    auto accumulateFiveTaps = [&](const double* rowPointer) {
        const float64x2_t xM2_M1 = vld1q_f64(rowPointer - 2), x0_1 = vld1q_f64(rowPointer + 0);
        const float64x2_t x2_3   = vld1q_f64(rowPointer + 2), x4_5 = vld1q_f64(rowPointer + 4);
        const float64x2_t x6_7   = vld1q_f64(rowPointer + 6), x8_9 = vld1q_f64(rowPointer + 8);
        const float64x2_t xM1_0 = vextq_f64(xM2_M1, x0_1, 1), x1_2 = vextq_f64(x0_1, x2_3, 1);
        const float64x2_t x3_4  = vextq_f64(x2_3,   x4_5, 1), x5_6 = vextq_f64(x4_5, x6_7, 1);
        const float64x2_t x7_8  = vextq_f64(x6_7,   x8_9, 1);
        accumulateTap(xM2_M1, x0_1, x2_3, x4_5);
        accumulateTap(xM1_0,  x1_2, x3_4, x5_6);
        accumulateTap(x0_1,   x2_3, x4_5, x6_7);
        accumulateTap(x1_2,   x3_4, x5_6, x7_8);
        accumulateTap(x2_3,   x4_5, x6_7, x8_9);
    };

    accumulateThreeTaps(centerInput - 2 * side);
    accumulateFiveTaps(centerInput - side);
    accumulateFiveTaps(centerInput);
    accumulateFiveTaps(centerInput + side);
    accumulateThreeTaps(centerInput + 2 * side);

    vst1q_f64(outputPointer + 0, sum0);
    vst1q_f64(outputPointer + 2, sum1);
    vst1q_f64(outputPointer + 4, sum2);
    vst1q_f64(outputPointer + 6, sum3);
}
#endif

// applyMatrix on a shared matrix, grid line by grid line with the six faces inside, so one line of
// `inner` stays in L1 while all six faces use it instead of the whole face streaming six times. Every
// row is written once with the sum forEachProduct gives it; only the order of the rows differs.
void applySharedMatrix(const StencilMatrix& matrix, const double* __restrict x, double* __restrict y) {
    const int side = matrix.side, run = side - 2 * Margin;
    auto write = [y](size_t r, double sum) { y[r] = sum; };
    for (size_t k = 0; k < matrix.seamRow.size(); ++k) y[size_t(matrix.seamRow[k])] = seamProduct(matrix, k, x);
    const double* line = matrix.inner.data();
    for (int j = Margin; j < side - Margin; ++j, line += size_t(Entries) * size_t(run))
        for (int face = 0; face < 6; ++face) {
            const double* v = line;
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0), first = (face * side + j) * side + Margin + i0;
                int i = 0;
#if defined(__aarch64__)
                for (; i + 8 <= count; i += 8) applyEightInteriorRowsNeon(v + i, count, x + first + i, side, y + first + i);
#endif
                blockProduct(matrix, v, count, first, x, i, write);
                v += Entries * count;
            }
        }
}

} // namespace

double shareFaceInteriors(SphereMatrix& matrix, int side, double limit) {
    const std::array<int, Entries> offset = offsetsFor(side);
    if (!regularOf(matrix, side, offset)) return -1;
    const int area = side * side, rows = int(matrix.diagonal.size());
    double largest = 0, differ = 0;
    for (double value : matrix.value) largest = std::max(largest, std::abs(value));
    for (int r = area; r < rows; ++r)
        if (insideFace(side, r))
            for (int e = 0; e < Entries; ++e)
                differ = std::max(differ, std::abs(matrix.value[size_t(matrix.start[size_t(r)] + e)] -
                                                   matrix.value[size_t(matrix.start[size_t(r % area)] + e)]));
    differ /= largest;
    if (differ > limit) return differ;
    for (int r = area; r < rows; ++r) {
        if (!insideFace(side, r)) continue;
        const int to = matrix.start[size_t(r)], from = matrix.start[size_t(r % area)];
        std::copy_n(matrix.value.begin() + from, Entries, matrix.value.begin() + to);
        matrix.diagonal[size_t(r)] = matrix.diagonal[size_t(r % area)];
    }
    // A seam row's entry in an interior column is that interior row's entry in the seam column.
    for (int r = 0; r < rows; ++r) {
        if (insideFace(side, r)) continue;
        for (int e = matrix.start[size_t(r)]; e < matrix.start[size_t(r) + 1]; ++e) {
            const int c = matrix.column[size_t(e)];
            if (!insideFace(side, c)) continue;
            const auto begin = matrix.column.begin() + matrix.start[size_t(c)], end = matrix.column.begin() + matrix.start[size_t(c) + 1];
            const auto found = std::lower_bound(begin, end, r);
            assert(found != end && *found == r);   // structurally symmetric
            matrix.value[size_t(e)] = matrix.value[size_t(found - matrix.column.begin())];
        }
    }
    return differ;
}

StencilMatrix makeStencilMatrix(const SphereMatrix& matrix, int side) {
    StencilMatrix split;
    split.side = side;
    split.diagonal = matrix.diagonal;
    split.offset = offsetsFor(side);
    const int rows = int(matrix.diagonal.size()), area = side * side;
    const bool regular = regularOf(matrix, side, split.offset);
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
    split.shared = regular;
    for (int r = area; r < rows && split.shared; ++r)
        if (insideFace(side, r))
            split.shared = std::equal(matrix.value.begin() + matrix.start[size_t(r)], matrix.value.begin() + matrix.start[size_t(r) + 1],
                                      matrix.value.begin() + matrix.start[size_t(r % area)]);
    // Values by block of up to Chunk rows of one grid line, entry by entry within the block, so the
    // product reads them in one sequential pass.
    const int run = side - 2 * Margin, faces = split.shared ? 1 : 6;
    split.inner.reserve(size_t(Entries) * size_t(split.interior / 6 * faces));
    for (int face = 0; face < faces && regular; ++face)
        for (int j = Margin; j < side - Margin; ++j)
            for (int i0 = 0; i0 < run; i0 += Chunk) {
                const int count = std::min(Chunk, run - i0), row = (face * side + j) * side + Margin + i0;
                for (int e = 0; e < Entries; ++e)
                    for (int i = 0; i < count; ++i) split.inner.push_back(matrix.value[size_t(matrix.start[size_t(row + i)]) + size_t(e)]);
            }
    return split;
}

void applyMatrix(const StencilMatrix& matrix, const std::vector<double>& field, std::vector<double>& result) {
    result.resize(matrix.diagonal.size());
    if (matrix.shared) {
        applySharedMatrix(matrix, field.data(), result.data());
        return;
    }
    double* y = result.data();
    forEachProduct(matrix, field.data(), [y](size_t r, double sum) { y[r] = sum; });
}

} // namespace derivean::sim
