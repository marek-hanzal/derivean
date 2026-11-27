#include "core/fn/Approx.hpp"
#include "core/fn/ApproxAngle.hpp"
#include "core/fn/ApproxErf.hpp"
#include "core/fn/Exact.hpp"
#include "core/fn/Hash.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <numbers>
#include <vector>

// The bounds and properties core/fn/Approx.hpp, ApproxAngle.hpp and ApproxErf.hpp promise, against
// CORE-MATH: random arguments over each function's domain, dense runs of neighbouring floats where
// the argument reduction changes branch, and the exact values the comments name. Every float is
// checked by `terra approx-check` on the heavy machine. 280 lines: one table of functions and the
// checks of the same three headers, cohesive.

namespace {

using namespace derivean::sim;

constexpr double DoubleBound = 1e-10;
constexpr int FloatUlps = 1;
constexpr int Samples = 20000;
constexpr int Run = 2000;   // neighbouring floats each side of a branch point

int failures = 0;

void expect(bool passed, const char* what, double at = 0, double got = 0, double want = 0) {
    if (passed) return;
    if (++failures <= 20) std::fprintf(stderr, "testApprox: %s at %a: %a, expected %a\n", what, at, got, want);
}

// Floats in order of value, −0 just below +0.
int64_t order(float v) {
    const int32_t i = std::bit_cast<int32_t>(v);
    return i < 0 ? -int64_t(i & INT32_MAX) - 1 : int64_t(i);
}
int64_t ulps(float a, float b) { return std::abs(order(a) - order(b)); }

bool normal(double v) { return std::isfinite(v) && std::abs(v) >= 0x1p-1022; }
bool normal(float v) { return std::isfinite(v) && std::abs(v) >= 0x1p-126f; }

// Where the exact result is not normal, what Approx.hpp promises: ±∞ itself, for ±0 a zero or a
// subnormal of the same sign, for a subnormal its sign (and, in float, the ulp bound, which may
// reach the smallest normal).
template<typename T> bool end(T got, T want) {
    if (std::signbit(got) != std::signbit(want)) return false;
    if (std::isinf(want)) return got == want;
    return std::isfinite(got) && (want != 0 || !normal(got));
}

void within(double got, double want, const char* what, double at) {
    if (want != want) return expect(got != got, what, at, got, want);
    if (!normal(want)) return expect(end(got, want), what, at, got, want);
    expect(std::abs(got - want) <= DoubleBound * std::abs(want), what, at, got, want);
}
void within(float got, float want, const char* what, double at) {
    if (want != want) return expect(got != got, what, at, got, want);
    if (!normal(want)) return expect(end(got, want) && (want == 0 || std::isinf(want) || ulps(got, want) <= FloatUlps), what, at, got, want);
    expect(ulps(got, want) <= FloatUlps, what, at, got, want);
}

// Uniform in [low, high), or log-uniform in magnitude when `logarithmic`, with a random sign when
// low < 0.
double draw(uint64_t salt, int i, double low, double high, bool logarithmic = false) {
    const double u = unit(0xa99, int64_t(salt), i);
    if (!logarithmic) return low + (high - low) * u;
    const double sign = low < 0 && unit(0xa99, int64_t(salt), i, 1) < 0.5 ? -1 : 1;
    const double a = std::abs(low < 0 ? 1e-300 : low), b = high;
    return sign * exact::exp2(exact::log2(std::max(a, 1e-300)) + (exact::log2(b) - exact::log2(std::max(a, 1e-300))) * u);
}

struct Unary {
    const char* name;
    double (*approxDouble)(double);
    float (*approxFloat)(float);
    double (*exactDouble)(double);
    float (*exactFloat)(float);
    double low, high;
    bool logarithmic;
    std::vector<double> branches;   // arguments where the reduction changes branch
    int monotone;                   // +1 increasing, −1 decreasing, 0 neither
};

void check(const Unary& f, uint64_t salt) {
    for (int i = 0; i < Samples; ++i) {
        const double x = draw(salt, i, f.low, f.high, f.logarithmic);
        within(f.approxDouble(x), f.exactDouble(x), f.name, x);
        within(f.approxFloat(float(x)), f.exactFloat(float(x)), f.name, x);
    }
    for (double branch : f.branches) {
        float x = float(branch);
        for (int i = 0; i < Run; ++i) x = std::nextafter(x, -std::numeric_limits<float>::infinity());
        float before = f.approxFloat(x);
        for (int i = 0; i < 2 * Run; ++i) {
            x = std::nextafter(x, std::numeric_limits<float>::infinity());
            const float now = f.approxFloat(x);
            within(now, f.exactFloat(x), f.name, x);
            within(f.approxDouble(x), f.exactDouble(x), f.name, x);
            const bool defined = now == now && before == before;   // outside the domain both are NaN
            if (defined && f.monotone > 0) expect(now >= before, "increasing", x, now, before);
            if (defined && f.monotone < 0) expect(now <= before, "decreasing", x, now, before);
            before = now;
        }
    }
}

std::vector<double> multiples(double step, double from, double to) {
    std::vector<double> at;
    for (double v = from; v <= to; v += step) at.push_back(v);
    return at;
}

} // namespace

void testApprox() {
    using std::numbers::ln2;
    using std::numbers::pi;
    const double root2 = std::numbers::sqrt2;
    const std::vector<double> halfLn2 = multiples(ln2, -40.5 * ln2, 40.5 * ln2);
    const std::vector<double> quarters = multiples(pi / 4, -12 * pi, 12 * pi);
    const std::vector<double> logBranches = {root2 / 4, root2 / 2, 0.5, 1, root2, 2, 2 * root2, 1e-3, 1e5};
    const Unary unary[] = {
        {"exp", approx::exp, approx::exp, exact::exp, exact::exp, -745, 709, false, halfLn2, +1},
        {"expm1", approx::expm1, approx::expm1, exact::expm1, exact::expm1, -40, 80, false, {-0.34657359027997264, 0.34657359027997264, 0}, +1},
        {"log", approx::log, approx::log, exact::log, exact::log, 1e-300, 1e300, true, logBranches, +1},
        {"log2", approx::log2, approx::log2, exact::log2, exact::log2, 1e-300, 1e300, true, logBranches, +1},
        {"log subnormal", approx::log, approx::log, exact::log, exact::log, 0x1p-1074, 0x1p-1022, true, {}, +1},
        {"log2 subnormal", approx::log2, approx::log2, exact::log2, exact::log2, 0x1p-1074, 0x1p-1022, true, {}, +1},
        {"cbrt subnormal", approx::cbrt, approx::cbrt, exact::cbrt, exact::cbrt, 0x1p-1074, 0x1p-1022, true, {}, +1},
        {"tanh", approx::tanh, approx::tanh, exact::tanh, exact::tanh, -25, 25, false, {-0.17328679513998632, 0, 0.17328679513998632, 9, 11}, +1},
        {"tanh near 0", approx::tanh, approx::tanh, exact::tanh, exact::tanh, -1e-3, 1e-3, true, {}, +1},
        {"atanh", approx::atanh, approx::atanh, exact::atanh, exact::atanh, -1, 1, false, {-0.17157287525380990, 0.17157287525380990, 0.999}, +1},
        {"atanh near 0", approx::atanh, approx::atanh, exact::atanh, exact::atanh, -1e-3, 1e-3, true, {}, +1},
        {"sinh", approx::sinh, approx::sinh, exact::sinh, exact::sinh, -715, 715, false, {-22, -0.34657359027997264, 0, 0.34657359027997264, 22, 709.78, 710.47}, +1},
        {"sinh near 0", approx::sinh, approx::sinh, exact::sinh, exact::sinh, -1e-3, 1e-3, true, {}, +1},
        {"erf", approx::erf, approx::erf, exact::erf, exact::erf, -7, 7, false, {-6, -3.25, -2, -1, 0, 1, 2, 3.25, 3.92, 6}, +1},
        {"erf near 0", approx::erf, approx::erf, exact::erf, exact::erf, -1e-3, 1e-3, true, {}, +1},
        {"cbrt", approx::cbrt, approx::cbrt, exact::cbrt, exact::cbrt, -1e300, 1e300, true, {1, 2, 4, 8, 0.5, 1e-3}, +1},
        {"sin", approx::sin, approx::sin, exact::sin, exact::sin, -1e5, 1e5, false, quarters, 0},
        {"sin near 0", approx::sin, approx::sin, exact::sin, exact::sin, -1, 1, true, {}, 0},
        {"cos", approx::cos, approx::cos, exact::cos, exact::cos, -1e5, 1e5, false, quarters, 0},
        {"sin far", approx::sin, approx::sin, exact::sin, exact::sin, 1e5, 0x1p17, false, {}, 0},
        {"cos far", approx::cos, approx::cos, exact::cos, exact::cos, -0x1p17, -1e5, false, {}, 0},
        {"tan", approx::tan, approx::tan, exact::tan, exact::tan, -1e5, 1e5, false, quarters, 0},
        {"tan near 0", approx::tan, approx::tan, exact::tan, exact::tan, -1, 1, true, {}, 0},
        {"atan", approx::atan, approx::atan, exact::atan, exact::atan, -1e30, 1e30, true, {-2.414213562373095, -1, -0.41421356237309515, 0, 0.41421356237309515, 1, 2.414213562373095}, +1},
        {"acos", approx::acos, approx::acos, exact::acos, exact::acos, -1, 1, false, {-1, -0.5, 0, 0.5, 1}, -1},
    };
    uint64_t salt = 1;
    for (const Unary& f : unary) check(f, salt++);

    // sincos gives what sin and cos give.
    for (int i = 0; i < Samples; ++i) {
        const double x = draw(salt, i, -1e5, 1e5);
        double s, c;
        approx::sincos(x, s, c);
        expect(s == approx::sin(x) && c == approx::cos(x), "sincos = sin, cos", x, s, c);
        float sf, cf;
        approx::sincos(float(x), sf, cf);
        expect(sf == approx::sin(float(x)) && cf == approx::cos(float(x)), "sincosf = sinf, cosf", x, sf, cf);
    }
    ++salt;

    // pow over the arguments that give finite, normal results, and its fixed exponents.
    for (int i = 0; i < Samples; ++i) {
        const double x = draw(salt, i, 1e-30, 1e30, true), y = draw(salt + 1, i, -20, 20);
        within(approx::pow(x, y), exact::pow(x, y), "pow", x);
        within(approx::pow(float(x), float(y)), exact::pow(float(x), float(y)), "powf", x);
        for (double m : {2.0, 3.0, 0.5, 1.0 / 3, 0.68, 1.5, -1.0}) {
            within(approx::pow(x, m), exact::pow(x, m), "pow fixed", x);
            within(approx::pow(float(x), float(m)), exact::pow(float(x), float(m)), "powf fixed", x);
        }
    }
    salt += 2;
    // pow where y log2 x nears ±1024 and ±1075: the largest finite doubles, the subnormals, and
    // the products that round to 1024 itself.
    for (int i = 0; i < Samples; ++i) {
        const double y = draw(salt, i, 0.5, 4), t = draw(salt + 1, i, 1023.5, 1024);
        const double x = exact::exp2(t / y);
        within(approx::pow(x, y), exact::pow(x, y), "pow near overflow", x);
        within(approx::pow(x, -y), exact::pow(x, -y), "pow near underflow", x);
        const double big = std::nextafter(std::numeric_limits<double>::max(), 0.0) * (1 - 0x1p-40 * unit(0xa99, int64_t(salt), i, 2));
        within(approx::pow(big, 1.0), exact::pow(big, 1.0), "pow of the largest doubles", big);
    }
    salt += 2;
    // Every zero, infinity, NaN, ±1 and sign among the arguments, as the C library has them.
    const double ends[] = {0.0, -0.0, 1.0, -1.0, 2.0, -2.0, 0.5, -0.5, 3.0, -3.0, 2.5, -2.5, 1e300, -1e300, 0x1p53, -0x1p53, 0x1p53 + 2, -(0x1p53 + 2),
                           std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    for (double x : ends)
        for (double y : ends) {
            // A normal result within the bound, every other one bit for bit (any NaN for NaN).
            const double got = approx::pow(x, y), want = exact::pow(x, y);
            if (normal(want)) within(got, want, "pow at the ends", x);
            else expect((got != got && want != want) || std::bit_cast<uint64_t>(got) == std::bit_cast<uint64_t>(want), "pow at the ends", x, got, y);
            const float gotf = approx::pow(float(x), float(y)), wantf = exact::pow(float(x), float(y));
            if (normal(wantf)) within(gotf, wantf, "powf at the ends", x);
            else expect((gotf != gotf && wantf != wantf) || std::bit_cast<uint32_t>(gotf) == std::bit_cast<uint32_t>(wantf), "powf at the ends", x, gotf, y);
        }
    // atan2 over all directions and magnitudes.
    for (int i = 0; i < Samples; ++i) {
        const double y = draw(salt, i, -1e10, 1e10, true), x = draw(salt + 1, i, -1e10, 1e10, true);
        within(approx::atan2(y, x), exact::atan2(y, x), "atan2", y);
        within(approx::atan2(float(y), float(x)), exact::atan2(float(y), float(x)), "atan2f", y);
        const double t = draw(salt + 2, i, 0, 1);   // around the kernel's branch at tan π/8
        within(approx::atan2(t, 1.0), exact::atan2(t, 1.0), "atan2 unit", t);
    }

    // Exact values and properties the comments promise.
    expect(approx::exp(0.0) == 1 && approx::exp(0.0f) == 1, "exp(0) = 1");
    expect(approx::exp(-800.0) == 0 && approx::exp(800.0) == std::numeric_limits<double>::infinity(), "exp beyond the doubles");
    expect(approx::exp(-700.0) > 0 && approx::exp(-87.0f) > 0, "exp positive");
    expect(approx::expm1(0.0) == 0, "expm1(0) = 0");
    expect(approx::log(1.0) == 0 && approx::log(1.0f) == 0, "log(1) = 0");
    for (int k = -1074; k <= 1023; ++k) expect(approx::log2(std::ldexp(1.0, k)) == k, "log2(2^k) = k", k);
    expect(approx::log(0.0) == -std::numeric_limits<double>::infinity() && approx::log(-1.0) != approx::log(-1.0), "log at 0 and below");
    expect(approx::pow(0.0, 0.68) == 0 && approx::pow(0.0f, 0.68f) == 0, "0^m = 0");
    expect(approx::pow(1.0, 0.68) == 1 && approx::pow(7.0, 0.0) == 1, "1^m = 1, x^0 = 1");
    expect(approx::pow(-2.0, 3.0) == -8 && approx::pow(-2.0, 2.0) == 4 && approx::pow(-2.0, 0.5) != approx::pow(-2.0, 0.5), "pow of negatives");
    expect(approx::tanh(0.0) == 0 && approx::tanh(30.0) == 1 && approx::tanh(-30.0f) == -1, "tanh at 0 and far");
    expect(approx::atanh(0.0) == 0 && approx::atanh(1.0) == std::numeric_limits<double>::infinity(), "atanh at 0 and 1");
    expect(approx::cbrt(1.0) == 1 && approx::cbrt(1.0f) == 1 && approx::cbrt(-8.0) == -2, "cbrt(1) = 1, cbrt(−8) = −2");
    for (int q = -358; q <= 341; ++q) expect(approx::cbrt(std::ldexp(1.0, 3 * q)) == std::ldexp(1.0, q), "cbrt(8^q) = 2^q", q);
    expect(approx::sin(0.0) == 0 && approx::cos(0.0) == 1 && approx::cos(0.0f) == 1, "sin(0) = 0, cos(0) = 1");
    // The sign of −0 kept by every odd function.
    for (const double z : {-0.0, 0.0}) {
        double s, c;
        approx::sincos(z, s, c);
        float sf, cf;
        approx::sincos(float(z), sf, cf);
        const bool kept = std::signbit(approx::tan(z)) == std::signbit(z) && std::signbit(approx::atan(z)) == std::signbit(z) &&
                          std::signbit(approx::sinh(z)) == std::signbit(z) && std::signbit(approx::erf(z)) == std::signbit(z) &&
                          std::signbit(approx::sin(z)) == std::signbit(z) && std::signbit(approx::sin(float(z))) == std::signbit(z) &&
                          std::signbit(s) == std::signbit(z) && std::signbit(sf) == std::signbit(z) && c == 1 && cf == 1 &&
                          std::signbit(approx::tanh(z)) == std::signbit(z) && std::signbit(approx::atanh(z)) == std::signbit(z) &&
                          std::signbit(approx::cbrt(z)) == std::signbit(z) && std::signbit(approx::expm1(z)) == std::signbit(z);
        expect(kept, "odd functions keep the sign of zero", z);
    }
    expect(approx::atan2(0.0, 1.0) == 0 && approx::atan2(1.0, 0.0) == 0x1.921fb54442d18p+0 && approx::atan2(0.0, -1.0) == 0x1.921fb54442d18p+1, "atan2 on the axes");
    expect(std::signbit(approx::atan2(-0.0, 1.0)) && approx::atan2(-0.0, -1.0) == -0x1.921fb54442d18p+1, "atan2 keeps the sign of zero");
    expect(approx::erf(0.0) == 0 && approx::erf(6.0) == 1 && approx::erf(-30.0f) == -1 && approx::erf(1e300) == 1, "erf at 0 and far");
    expect(approx::atan(std::numeric_limits<double>::infinity()) == 0x1.921fb54442d18p+0 && approx::atan(1.0) == 0x1.921fb54442d18p-1, "atan at ∞ and 1");
    expect(approx::sinh(800.0) == std::numeric_limits<double>::infinity() && approx::sinh(-710.0) > -std::numeric_limits<double>::infinity(), "sinh as far as it is finite");
    for (const double edge : {0x1.633ce8fb9f87dp+9, 0x1.633ce8fb9f87ep+9, -0x1.633ce8fb9f87dp+9, -0x1.633ce8fb9f87ep+9})
        within(approx::sinh(edge), exact::sinh(edge), "sinh at the overflow threshold", edge);
    // tan increases between its poles: on a fine grid, wherever no pole lies between neighbours.
    for (int i = -2000000; i < 2000000; ++i) {
        const double x = i * 1e-5, next = (i + 1) * 1e-5;
        if (std::floor(x / pi + 0.5) != std::floor(next / pi + 0.5)) continue;
        expect(approx::tan(next) >= approx::tan(x), "tan increasing between poles", x);
    }
    expect(approx::tan(0.0) == 0 && approx::sinh(0.0) == 0, "tan(0) = 0, sinh(0) = 0");
    expect(approx::acos(1.0) == 0 && approx::acos(-1.0) == 0x1.921fb54442d18p+1 && approx::acos(2.0) != approx::acos(2.0), "acos at the ends");
    for (int i = 0; i < Samples; ++i) {
        const double x = draw(salt + 3, i, -1e5, 1e5), t = draw(salt + 4, i, -1e3, 1e3);
        expect(std::abs(approx::sin(x)) <= 1 && std::abs(approx::cos(x)) <= 1, "|sin|, |cos| ≤ 1", x);
        const auto same = [](double a, double b) { return std::bit_cast<uint64_t>(a) == std::bit_cast<uint64_t>(b); };
        expect(same(approx::sin(-x), -approx::sin(x)) && same(approx::cos(-x), approx::cos(x)), "sin odd, cos even", x);
        expect(std::abs(approx::tanh(t)) <= 1 && same(approx::tanh(-t), -approx::tanh(t)), "tanh odd, |tanh| ≤ 1", t);
        expect(same(approx::cbrt(-t), -approx::cbrt(t)), "cbrt odd", t);
        expect(same(approx::sinh(-t), -approx::sinh(t)) && same(approx::tan(-x), -approx::tan(x)), "sinh, tan odd", t);
        expect(same(approx::erf(-t * 1e-2), -approx::erf(t * 1e-2)) && std::abs(approx::erf(t * 1e-2)) <= 1, "erf odd, |erf| ≤ 1", t);
        expect(same(approx::atan(-x), -approx::atan(x)) && std::abs(approx::atan(x)) <= 0x1.921fb54442d18p+0, "atan odd, in [−π/2, π/2]", x);
        const double a = approx::atan2(x, t);
        expect(a >= -0x1.921fb54442d18p+1 && a <= 0x1.921fb54442d18p+1 && (a >= 0) == (x >= 0), "atan2 in [−π, π], upper half for y ≥ 0", x);
        const double c = approx::acos(std::clamp(x * 1e-5, -1.0, 1.0));
        expect(c >= 0 && c <= 0x1.921fb54442d18p+1, "acos in [0, π]", x);
    }
    if (failures) {
        std::fprintf(stderr, "testApprox: %d failures\n", failures);
        std::exit(1);
    }
}
