#pragma once

// The arm64 kernels of the shared stiffness product (StencilMatrix.cpp), one block of consecutive
// interior rows at a time: 8 rows in double, 16 in float. Each of the five grid lines dj = −2..2 is
// loaded once and the neighbours di are formed in registers by vextq instead of reloading the field
// per entry. The taps run in offsetsFor's order (dj outer, di inner), so every sum adds its 21
// products in column order as the compressed row does; vfmaq fuses each product with its sum, which
// is what clang makes of `sum += value * x` under -ffp-contract=on (Arm Neon Intrinsics Reference and
// the Clang manual, docs/credits/libraries.md, platform documentation). Every load stays within
// columns −2 .. rows + 1 of the block's own grid lines.

#if defined(__aarch64__)

#include <arm_neon.h>

namespace derivean::sim {

template <class Real> constexpr int NeonRows = 0;
template <> inline constexpr int NeonRows<double> = 8;
template <> inline constexpr int NeonRows<float> = 16;

// `values` holds entry e of row u at values[e · stride + u]; `x` points at the block's first row.
inline void productNeon(const double* __restrict values, int stride, const double* __restrict x, int side, double* __restrict y) {
    float64x2_t sum0 = vdupq_n_f64(0.0), sum1 = sum0, sum2 = sum0, sum3 = sum0;
    const double* weight = values;
    auto tap = [&](float64x2_t a, float64x2_t b, float64x2_t c, float64x2_t d) {
        sum0 = vfmaq_f64(sum0, vld1q_f64(weight + 0), a);
        sum1 = vfmaq_f64(sum1, vld1q_f64(weight + 2), b);
        sum2 = vfmaq_f64(sum2, vld1q_f64(weight + 4), c);
        sum3 = vfmaq_f64(sum3, vld1q_f64(weight + 6), d);
        weight += stride;
    };
    auto line = [&](const double* row, bool wide) {
        const float64x2_t m2 = vld1q_f64(row - 2), p0 = vld1q_f64(row), p2 = vld1q_f64(row + 2), p4 = vld1q_f64(row + 4);
        const float64x2_t p6 = vld1q_f64(row + 6), p8 = vld1q_f64(row + 8);
        const float64x2_t m1 = vextq_f64(m2, p0, 1), p1 = vextq_f64(p0, p2, 1), p3 = vextq_f64(p2, p4, 1);
        const float64x2_t p5 = vextq_f64(p4, p6, 1), p7 = vextq_f64(p6, p8, 1);
        if (wide) tap(m2, p0, p2, p4);
        tap(m1, p1, p3, p5);
        tap(p0, p2, p4, p6);
        tap(p1, p3, p5, p7);
        if (wide) tap(p2, p4, p6, p8);
    };
    line(x - 2 * side, false);
    line(x - side, true);
    line(x, true);
    line(x + side, true);
    line(x + 2 * side, false);
    vst1q_f64(y + 0, sum0);
    vst1q_f64(y + 2, sum1);
    vst1q_f64(y + 4, sum2);
    vst1q_f64(y + 6, sum3);
}

inline void productNeon(const float* __restrict values, int stride, const float* __restrict x, int side, float* __restrict y) {
    float32x4_t sum0 = vdupq_n_f32(0.0f), sum1 = sum0, sum2 = sum0, sum3 = sum0;
    const float* weight = values;
    auto tap = [&](float32x4_t a, float32x4_t b, float32x4_t c, float32x4_t d) {
        sum0 = vfmaq_f32(sum0, vld1q_f32(weight + 0), a);
        sum1 = vfmaq_f32(sum1, vld1q_f32(weight + 4), b);
        sum2 = vfmaq_f32(sum2, vld1q_f32(weight + 8), c);
        sum3 = vfmaq_f32(sum3, vld1q_f32(weight + 12), d);
        weight += stride;
    };
    // Columns −2 .. 17 of a line in five registers; neighbour di of rows 4k .. 4k + 3 is the window
    // starting di + 2 lanes into register k.
    auto line = [&](const float* row, bool wide) {
        const float32x4_t v0 = vld1q_f32(row - 2), v1 = vld1q_f32(row + 2), v2 = vld1q_f32(row + 6), v3 = vld1q_f32(row + 10);
        const float32x4_t v4 = vld1q_f32(row + 14);
        if (wide) tap(v0, v1, v2, v3);
        tap(vextq_f32(v0, v1, 1), vextq_f32(v1, v2, 1), vextq_f32(v2, v3, 1), vextq_f32(v3, v4, 1));
        tap(vextq_f32(v0, v1, 2), vextq_f32(v1, v2, 2), vextq_f32(v2, v3, 2), vextq_f32(v3, v4, 2));
        tap(vextq_f32(v0, v1, 3), vextq_f32(v1, v2, 3), vextq_f32(v2, v3, 3), vextq_f32(v3, v4, 3));
        if (wide) tap(v1, v2, v3, v4);
    };
    line(x - 2 * side, false);
    line(x - side, true);
    line(x, true);
    line(x + side, true);
    line(x + 2 * side, false);
    vst1q_f32(y + 0, sum0);
    vst1q_f32(y + 4, sum1);
    vst1q_f32(y + 8, sum2);
    vst1q_f32(y + 12, sum3);
}

} // namespace derivean::sim

#endif
