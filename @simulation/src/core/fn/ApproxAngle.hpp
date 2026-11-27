#pragma once

#include "core/constant/ApproxCoefficients.hpp"
#include "core/fn/Approx.hpp"
#include "core/fn/Exact.hpp"

#include <cmath>
#include <cstdint>

// Sine, cosine and arcs for the simulation's hot calls, built and bounded as core/fn/Approx.hpp
// says: double 1e-10 relative, float 1 ulp, the same bits on macOS and Windows.

namespace derivean::sim::approx {

namespace detail {

inline constexpr double Pi = 0x1.921fb54442d18p+1;
inline constexpr double HalfPi = 0x1.921fb54442d18p+0;
inline constexpr double QuarterPi = 0x1.921fb54442d18p-1;
inline constexpr double TwoOverPi = 0x1.45f306dc9c883p-1;
// π/2 in three parts, each the rounding of what the ones before leave: x − q π/2 by three fmas
// keeps its relative accuracy while |q| ≤ 2^17 (Cody and Waite, docs/credits/numerics.md).
inline constexpr double HalfPi2 = 0x1.1a62633145c07p-54;
inline constexpr double HalfPi3 = -0x1.f1976b7ed8fbcp-110;
// Beyond this |x| the angles go to CORE-MATH: no hot call reaches it.
inline constexpr double Reduced = 0x1p17;

inline double sinKernel(double r) { return r * horner(coefficients::SinOdd, r * r); }
inline double cosKernel(double r) { return std::fma(r * r, horner(coefficients::CosOne, r * r), 1.0); }

// x = q π/2 + r with |r| ≤ π/4 (a hair more where q rounds), and q mod 4.
inline double reduce(double x, int& quadrant) {
    const double q = std::nearbyint(x * TwoOverPi);
    quadrant = int(int64_t(q) & 3);
    if (q == 0) return x;   // exact, and keeps the sign of −0
    return std::fma(-q, HalfPi3, std::fma(-q, HalfPi2, std::fma(-q, HalfPi, x)));
}

// atan(t) for 0 ≤ t ≤ 1: the kernel on [0, tan π/8], and π/4 + atan((t − 1) / (t + 1)) above.
inline double atanUnit(double t) {
    if (t <= 0.41421356237309515) return t * horner(coefficients::AtanOdd, t * t);
    const double u = (t - 1) / (t + 1);
    return std::fma(u, horner(coefficients::AtanOdd, u * u), QuarterPi);
}

inline double asinHalf(double x) { return x * horner(coefficients::AsinOdd, x * x); }

} // namespace detail

// Sine. Odd, |sin| ≤ 1, sin(0) = 0.
inline double sin(double x) {
    if (!(std::abs(x) < detail::Reduced)) return exact::sin(x);
    int quadrant;
    const double r = detail::reduce(x, quadrant);
    const double v = quadrant & 1 ? detail::cosKernel(r) : detail::sinKernel(r);
    return quadrant & 2 ? -v : v;
}
inline float sin(float x) { return float(sin(double(x))); }

// Cosine. Even, |cos| ≤ 1, cos(0) = 1.
inline double cos(double x) {
    if (!(std::abs(x) < detail::Reduced)) return exact::cos(x);
    int quadrant;
    const double r = detail::reduce(x, quadrant);
    const double v = quadrant & 1 ? detail::sinKernel(r) : detail::cosKernel(r);
    return (quadrant + 1) & 2 ? -v : v;
}
inline float cos(float x) { return float(cos(double(x))); }

// Sine and cosine of one angle, the same values sin and cos give.
inline void sincos(double x, double& sine, double& cosine) {
    if (!(std::abs(x) < detail::Reduced)) {
        exact::sincos(x, sine, cosine);
        return;
    }
    int quadrant;
    const double r = detail::reduce(x, quadrant);
    const double s = detail::sinKernel(r), c = detail::cosKernel(r);
    const double a = quadrant & 1 ? c : s, b = quadrant & 1 ? s : c;
    sine = quadrant & 2 ? -a : a;
    cosine = (quadrant + 1) & 2 ? -b : b;
}
inline void sincos(float x, float& sine, float& cosine) {
    double s, c;
    sincos(double(x), s, c);
    sine = float(s);
    cosine = float(c);
}

// The angle of (x, y) from the x axis. In [−π, π], in the quadrant of (x, y); on the axes exactly
// 0, ±π/2 and ±π as rounded to double; the sign of y's zero kept, as the C library does.
inline double atan2(double y, double x) {
    if (x != x || y != y) return x + y;
    const double ax = std::abs(x), ay = std::abs(y);
    double a;
    if (ax == detail::Infinity && ay == detail::Infinity) a = detail::QuarterPi;
    else if (ay == 0 && ax == 0) a = 0;
    else if (ay <= ax) a = detail::atanUnit(ay / ax);
    else a = detail::HalfPi - detail::atanUnit(ax / ay);
    if (std::signbit(x)) a = detail::Pi - a;
    return std::copysign(a, y);
}
inline float atan2(float y, float x) { return float(atan2(double(y), double(x))); }

// Arc cosine. In [0, π], decreasing to within the bound, acos(1) = 0 and acos(−1) = π as rounded
// to double; NaN beyond ±1.
inline double acos(double x) {
    const double a = std::abs(x);
    if (!(a <= 1)) return detail::Nan;
    if (a <= 0.5) return detail::HalfPi - detail::asinHalf(x);
    const double half = 2 * detail::asinHalf(std::sqrt(0.5 * (1 - a)));
    return x > 0 ? half : detail::Pi - half;
}
inline float acos(float x) { return float(acos(double(x))); }

template<typename T> void sin(T) = delete;
template<typename T> void cos(T) = delete;
template<typename T> void acos(T) = delete;
template<typename A, typename B> void atan2(A, B) = delete;
template<typename T, typename S> void sincos(T, S&, S&) = delete;

} // namespace derivean::sim::approx
