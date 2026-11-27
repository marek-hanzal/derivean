#include "core/fn/Exact.hpp"
#include "core/fn/Hash.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

using namespace derivean::sim;

void expect(bool passed, const char* message) {
    if (passed) return;
    std::fprintf(stderr, "testExact: %s\n", message);
    std::exit(1);
}

// Arguments for one function: uniform in [low, high), drawn from the seeded hash, and the same
// on every platform.
double argument(uint64_t function, int64_t index, double low, double high) {
    return ranged(hash(0x1ee7, int64_t(function), index), low, high);
}

struct Digest {
    uint64_t state = 0;
    void add(double value) { state = scramble(state ^ std::bit_cast<uint64_t>(value)); }
    void add(float value) { state = scramble(state ^ std::bit_cast<uint32_t>(value)); }
};

// Every function on 512 arguments per precision over its range of use and past it. Correct
// rounding makes each result the exact value rounded, so one digest holds on every platform.
uint64_t digest() {
    constexpr int Count = 512;
    Digest d;
    uint64_t f = 0;
    const auto unary = [&](auto function, double low, double high) {
        ++f;
        for (int i = 0; i < Count; ++i) {
            const double x = argument(f, i, low, high);
            d.add(function(x));
            d.add(function(float(x)));
        }
    };
    const auto binary = [&](auto function, double low, double high, double low2, double high2) {
        ++f;
        for (int i = 0; i < Count; ++i) {
            const double a = argument(f, 2 * i, low, high), b = argument(f, 2 * i + 1, low2, high2);
            d.add(function(a, b));
            d.add(function(float(a), float(b)));
        }
    };
    unary([](auto x) { return exact::sin(x); }, -100, 100);
    unary([](auto x) { return exact::cos(x); }, -100, 100);
    unary([](auto x) { return exact::tan(x); }, -1.5, 1.5);
    unary([](auto x) { return exact::asin(x); }, -1, 1);
    unary([](auto x) { return exact::acos(x); }, -1, 1);
    unary([](auto x) { return exact::atan(x); }, -50, 50);
    unary([](auto x) { return exact::sinh(x); }, -20, 20);
    unary([](auto x) { return exact::cosh(x); }, -20, 20);
    unary([](auto x) { return exact::tanh(x); }, -10, 10);
    unary([](auto x) { return exact::atanh(x); }, -0.999, 0.999);
    unary([](auto x) { return exact::exp(x); }, -80, 80);
    unary([](auto x) { return exact::exp2(x); }, -120, 120);
    unary([](auto x) { return exact::exp10(x); }, -30, 30);
    unary([](auto x) { return exact::expm1(x); }, -5, 5);
    unary([](auto x) { return exact::log(x); }, 1e-6, 1e6);
    unary([](auto x) { return exact::log2(x); }, 1e-6, 1e6);
    unary([](auto x) { return exact::log10(x); }, 1e-6, 1e6);
    unary([](auto x) { return exact::log1p(x); }, -0.9, 10);
    unary([](auto x) { return exact::cbrt(x); }, -1e4, 1e4);
    unary([](auto x) { return exact::erf(x); }, -4, 4);
    unary([](auto x) { return exact::erfc(x); }, -4, 9);
    binary([](auto y, auto x) { return exact::atan2(y, x); }, -10, 10, -10, 10);
    binary([](auto x, auto y) { return exact::pow(x, y); }, 0, 50, -4, 4);
    binary([](auto x, auto y) { return exact::hypot(x, y); }, -1e3, 1e3, -1e3, 1e3);
    ++f;
    for (int i = 0; i < Count; ++i) {
        const double x = argument(f, i, -100, 100);
        double s, c;
        float sf, cf;
        exact::sincos(x, s, c);
        exact::sincos(float(x), sf, cf);
        d.add(s), d.add(c), d.add(sf), d.add(cf);
    }
    return d.state;
}

} // namespace

void testExact() {
    // Results that are exact numbers, so their correct rounding is the number itself.
    expect(exact::exp(0.0) == 1.0 && exact::exp(0.0f) == 1.0f, "exp(0) = 1");
    expect(exact::log(1.0) == 0.0 && exact::log(1.0f) == 0.0f, "log(1) = 0");
    expect(exact::log2(8.0) == 3.0 && exact::log2(8.0f) == 3.0f, "log2(8) = 3");
    expect(exact::exp2(-3.0) == 0.125 && exact::exp2(-3.0f) == 0.125f, "exp2(-3) = 1/8");
    expect(exact::exp10(2.0) == 100.0 && exact::exp10(2.0f) == 100.0f, "exp10(2) = 100");
    expect(exact::cbrt(27.0) == 3.0 && exact::cbrt(-27.0f) == -3.0f, "cbrt(27) = 3");
    expect(exact::pow(2.0, 10.0) == 1024.0 && exact::pow(1.5f, 2.0f) == 2.25f, "exact powers");
    expect(exact::hypot(3.0, 4.0) == 5.0 && exact::hypot(3.0f, 4.0f) == 5.0f, "hypot(3, 4) = 5");
    expect(exact::sin(0.0) == 0.0 && exact::cos(0.0f) == 1.0f && exact::atan2(0.0, 1.0) == 0.0, "trigonometry at zero");
    // Correctly rounded square roots are IEEE's own, so pow(x, 1/2) must equal sqrt(x) on every
    // positive finite x, in both precisions.
    for (int i = 0; i < 4096; ++i) {
        const double x = std::ldexp(1.0 + unit(0x5eed, i), int(hash(0x5eed, i, 1) % 400) - 200);
        expect(exact::pow(x, 0.5) == std::sqrt(x), "pow(x, 0.5) = sqrt(x)");
        const float xf = std::ldexp(1.0f + float(unit(0x5eed, i, 2)), int(hash(0x5eed, i, 3) % 250) - 125);
        expect(exact::pow(xf, 0.5f) == std::sqrt(xf), "powf(x, 0.5) = sqrtf(x)");
    }
    // One digest of every function, the same on macOS and Windows.
    const uint64_t got = digest();
    constexpr uint64_t Expected = 0x75033619684861fcULL;
    if (got != Expected) {
        std::fprintf(stderr, "testExact: digest %016llx, expected %016llx\n", (unsigned long long)got, (unsigned long long)Expected);
        std::exit(1);
    }
}
