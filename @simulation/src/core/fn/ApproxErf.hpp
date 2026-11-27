#pragma once

#include "core/constant/ApproxCoefficients.hpp"
#include "core/fn/Approx.hpp"

#include <algorithm>
#include <cmath>

// The error function for the simulation's hot calls, built and bounded as core/fn/Approx.hpp says:
// double 1e-10 relative, float 1 ulp, the same bits on macOS and Windows. Its four polynomials are
// interpolated by `terra approx` (Trefethen, docs/credits/numerics.md).

namespace derivean::sim::approx {

// Error function. Odd, increasing to within the bound, |erf| ≤ 1, erf(0) = 0, and ±1 from ±6 on,
// where erf rounds to ±1 in double. x · p(x²) up to 1, then one polynomial in |x| − c on each of
// [1, 2], [2, 3.25] and [3.25, 6] with c the middle; |x| − c is exact there.
inline double erf(double x) {
    const double a = std::abs(x);
    if (!(a < 6)) return a == a ? std::copysign(1.0, x) : x;
    if (a <= 1) return x * detail::horner(coefficients::ErfOdd, x * x);
    double v;
    if (a <= 2) v = detail::horner(coefficients::ErfNear, a - 1.5);
    else if (a <= 3.25) v = detail::horner(coefficients::ErfMiddle, a - 2.625);
    else v = detail::horner(coefficients::ErfFar, a - 4.625);
    return std::copysign(std::min(v, 1.0), x);
}
inline float erf(float x) { return float(erf(double(x))); }

template<typename T> void erf(T) = delete;

} // namespace derivean::sim::approx
