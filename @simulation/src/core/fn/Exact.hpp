#pragma once

// Correctly rounded maths from CORE-MATH (docs/credits/libraries.md, built by @simulation/xmake.lua):
// each result is the exact value rounded to nearest, the same bits on every platform and compiler,
// where the platform's libm differs by a few ulp between macOS and Windows. That holds in the
// default floating-point environment (round to nearest, no flush to zero) and for every result but
// NaN: a NaN the hardware makes has its sign bit set on x86-64 and clear on arm64. It costs more than the
// platform's functions, so it serves calls that are rare or that a long integration accumulates.
//
// Overloads exist for float and double only. Any other argument type, or a mix of float and double,
// is a compile error, so that no call picks its precision by conversion.

#define DERIVEAN_EXACT_UNARY(X) \
    X(sin) X(cos) X(tan) X(asin) X(acos) X(atan) X(sinh) X(cosh) X(tanh) X(atanh) \
    X(exp) X(exp2) X(exp10) X(expm1) X(log) X(log2) X(log10) X(log1p) X(cbrt) X(erf) X(erfc)
#define DERIVEAN_EXACT_BINARY(X) X(atan2) X(pow) X(hypot)

extern "C" {
#define DERIVEAN_DECLARE_UNARY(name) double cr_##name(double); float cr_##name##f(float);
#define DERIVEAN_DECLARE_BINARY(name) double cr_##name(double, double); float cr_##name##f(float, float);
DERIVEAN_EXACT_UNARY(DERIVEAN_DECLARE_UNARY)
DERIVEAN_EXACT_BINARY(DERIVEAN_DECLARE_BINARY)
#undef DERIVEAN_DECLARE_UNARY
#undef DERIVEAN_DECLARE_BINARY
void cr_sincos(double, double*, double*);
void cr_sincosf(float, float*, float*);
}

namespace derivean::sim::exact {

#define DERIVEAN_DEFINE_UNARY(name) \
    inline double name(double x) noexcept { return cr_##name(x); } \
    inline float name(float x) noexcept { return cr_##name##f(x); } \
    template<typename T> void name(T) = delete;
#define DERIVEAN_DEFINE_BINARY(name) \
    inline double name(double a, double b) noexcept { return cr_##name(a, b); } \
    inline float name(float a, float b) noexcept { return cr_##name##f(a, b); } \
    template<typename A, typename B> void name(A, B) = delete;
DERIVEAN_EXACT_UNARY(DERIVEAN_DEFINE_UNARY)
DERIVEAN_EXACT_BINARY(DERIVEAN_DEFINE_BINARY)
#undef DERIVEAN_DEFINE_UNARY
#undef DERIVEAN_DEFINE_BINARY

// Sine and cosine of one angle.
inline void sincos(double x, double& sine, double& cosine) noexcept { cr_sincos(x, &sine, &cosine); }
inline void sincos(float x, float& sine, float& cosine) noexcept { cr_sincosf(x, &sine, &cosine); }
template<typename T, typename S> void sincos(T, S&, S&) = delete;

} // namespace derivean::sim::exact

#undef DERIVEAN_EXACT_UNARY
#undef DERIVEAN_EXACT_BINARY
