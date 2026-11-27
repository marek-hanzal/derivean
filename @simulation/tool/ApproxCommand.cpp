#include "Arguments.hpp"
#include "core/fn/Exact.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <numbers>
#include <string>
#include <vector>

// `terra approx`: the polynomial coefficients behind core/fn/Approx.hpp, printed as the header
// core/constant/ApproxCoefficients.hpp. Each kernel is interpolated at Chebyshev nodes on its
// interval (Trefethen, docs/credits/numerics.md), which comes within a small factor of the
// minimax polynomial of the same degree, then turned into powers of its variable for Horner's
// rule. The reference is CORE-MATH (core/fn/Exact.hpp). Each kernel's relative error is measured
// on a dense grid, with the coefficients as rounded to double and evaluated as Approx.hpp does,
// and the command fails when one exceeds its bound. The output is the same text on macOS and
// Windows, line ends aside: every transcendental value comes from CORE-MATH, every a · b + c is an
// explicit fma, and the hexadecimal doubles are written here rather than by the C library's %a,
// whose digits differ between the platforms.

namespace {

using namespace derivean::sim;

// How a kernel's polynomial stands for its function f on [low, high]:
//   Plain   f(x) ≈ p(x)
//   Odd     f(x) ≈ x · p(x²), for odd f with f(0) = 0, so the relative error holds down to 0
//   Even    f(x) ≈ p(x²)
//   Rest    f(x) ≈ x · p(x), for f with f(0) = 0 that is not odd (e^x − 1)
//   Cosine  f(x) ≈ 1 + x² · p(x²), for even f with f(0) = 1 exactly
enum class Form { Plain, Odd, Even, Rest, Cosine };

struct Kernel {
    const char* name;
    const char* meaning;
    Form form;
    double low, high;    // the interval of x
    int degree;          // of p at most; the lowest degree that meets the bound is taken
    double bound;        // largest relative error allowed on the interval
    std::function<double(double)> f;
    // p's own target as a function of its variable (x, or x² for the squared forms), where the
    // quotient by f loses digits near zero; empty to derive it from f.
    std::function<double(double)> g = {};
};

// p as powers of its variable, lowest first.
using Polynomial = std::vector<double>;

double horner(const Polynomial& p, double t) {
    double sum = p.back();
    for (size_t i = p.size() - 1; i-- > 0;) sum = std::fma(sum, t, p[i]);
    return sum;
}

double evaluate(const Kernel& k, const Polynomial& p, double x) {
    switch (k.form) {
    case Form::Plain: return horner(p, x);
    case Form::Odd: return x * horner(p, x * x);
    case Form::Even: return horner(p, x * x);
    case Form::Rest: return x * horner(p, x);
    case Form::Cosine: return std::fma(x * x, horner(p, x * x), 1.0);
    }
    return 0;
}

// The function p stands for, of its own variable t.
// At zero itself, where the quotients below are 0 / 0, the point is moved to `Near`, so close that
// the quotient is its limit in double.
constexpr double Near = 1e-150;
double target(const Kernel& k, double t) {
    if (k.g) return k.g(t);
    switch (k.form) {
    case Form::Plain: return k.f(t);
    case Form::Rest: return t == 0 ? k.f(Near) / Near : k.f(t) / t;
    default: break;
    }
    const double x = std::max(std::sqrt(t), Near);
    if (k.form == Form::Even) return k.f(x);
    if (k.form == Form::Odd) return k.f(x) / x;
    return (k.f(x) - 1) / (x * x);   // Cosine
}

Polynomial interpolate(const Kernel& k, int degree) {
    // The variable's interval: x itself, or x² for the odd and even forms (low ≥ 0 there).
    const bool squared = k.form == Form::Odd || k.form == Form::Even || k.form == Form::Cosine;
    const double a = squared ? k.low * k.low : k.low;
    const double b = squared ? k.high * k.high : k.high;
    const int n = degree + 1;
    // Chebyshev coefficients from the values at the n Chebyshev points of the first kind.
    std::vector<double> c(size_t(n), 0.0);
    for (int j = 0; j < n; ++j) {
        const double angle = std::numbers::pi * (j + 0.5) / n;
        const double value = target(k, std::fma(0.5 * (b - a), exact::cos(angle), 0.5 * (a + b)));
        for (int m = 0; m < n; ++m) c[size_t(m)] = std::fma(value, exact::cos(m * angle), c[size_t(m)]);
    }
    for (int m = 0; m < n; ++m) c[size_t(m)] *= (m == 0 ? 1.0 : 2.0) / n;
    // Σ c_m T_m(u) with u = (2t − a − b) / (b − a), as powers of t: the recurrence
    // T_{m+1} = 2u T_m − T_{m−1} carried out on polynomials in t.
    const double scale = 2 / (b - a), shift = -(a + b) / (b - a);   // u = scale · t + shift
    Polynomial previous{1.0}, current{shift, scale}, sum(size_t(n), 0.0);
    sum[0] += c[0];
    if (n > 1) for (size_t i = 0; i < current.size(); ++i) sum[i] = std::fma(c[1], current[i], sum[i]);
    for (int m = 2; m < n; ++m) {
        Polynomial next(current.size() + 1, 0.0);
        for (size_t i = 0; i < current.size(); ++i) {
            next[i] = std::fma(2 * shift, current[i], next[i]);
            next[i + 1] = std::fma(2 * scale, current[i], next[i + 1]);
        }
        for (size_t i = 0; i < previous.size(); ++i) next[i] -= previous[i];
        for (size_t i = 0; i < next.size(); ++i) sum[i] = std::fma(c[size_t(m)], next[i], sum[i]);
        previous = std::move(current);
        current = std::move(next);
    }
    return sum;
}

// Largest relative error on a grid of `samples` points and the interval's ends.
double worst(const Kernel& k, const Polynomial& p, int samples) {
    double most = 0;
    for (int i = 0; i <= samples; ++i) {
        const double x = k.low + (k.high - k.low) * i / samples;
        const double exact = k.f(x);
        if (exact == 0) continue;
        most = std::max(most, std::abs(evaluate(k, p, x) - exact) / std::abs(exact));
    }
    return most;
}

// A finite double as C reads it back exactly, in the shortest form of macOS's %a: 0x1.8p+1 for 3.
std::string hexadecimal(double v) {
    const uint64_t bits = std::bit_cast<uint64_t>(v);
    std::string text = bits >> 63 ? "-0x" : "0x";
    const int biased = int((bits >> 52) & 0x7ff);
    uint64_t fraction = bits & 0x000fffffffffffffULL;
    if (biased == 0 && fraction == 0) return text + "0p+0";
    text += biased == 0 ? "0" : "1";
    int digits = 13;
    while (digits > 0 && (fraction & 0xf) == 0) fraction >>= 4, --digits;
    if (digits > 0) {
        char buffer[16];
        std::snprintf(buffer, sizeof buffer, ".%0*llx", digits, (unsigned long long)fraction);
        text += buffer;
    }
    const int exponent = biased == 0 ? -1022 : biased - 1023;
    return text + (exponent < 0 ? "p-" : "p+") + std::to_string(std::abs(exponent));
}

const std::vector<Kernel>& kernels() {
    static const std::vector<Kernel> all = {
#include "ApproxKernels.inc"
    };
    return all;
}

} // namespace

int approxCommand(const Arguments& arguments) {
    const int samples = int(arguments.number("samples", 2e6));
    bool fine = true;
    std::printf("#pragma once\n\n");
    std::printf("// Generated by `terra approx` (@simulation/tool/ApproxCommand.cpp); do not edit. Coefficients of the\n");
    std::printf("// polynomials of core/fn/Approx.hpp and ApproxAngle.hpp, lowest power first, as hexadecimal doubles\n");
    std::printf("// so that they are read back exactly. The error is the largest relative error measured against\n");
    std::printf("// CORE-MATH.\n\n");
    std::printf("namespace derivean::sim::approx::coefficients {\n");
    for (const Kernel& k : kernels()) {
        Polynomial p;
        double error = 0;
        for (int degree = 1; degree <= k.degree; ++degree) {
            p = interpolate(k, degree);
            error = worst(k, p, samples);
            if (error <= k.bound) break;
        }
        fine = fine && error <= k.bound;
        std::printf("\n// %s on [%.17g, %.17g], degree %d: relative error %.3g (bound %.3g)\n", k.meaning, k.low, k.high, int(p.size()) - 1, error, k.bound);
        std::printf("inline constexpr double %s[] = {", k.name);
        for (size_t i = 0; i < p.size(); ++i) std::printf("%s%s", i ? ", " : "", hexadecimal(p[i]).c_str());
        std::printf("};\n");
        if (error > k.bound) std::fprintf(stderr, "terra approx: %s has relative error %.3g over its bound %.3g\n", k.name, error, k.bound);
    }
    std::printf("\n} // namespace derivean::sim::approx::coefficients\n");
    return fine ? 0 : 1;
}
