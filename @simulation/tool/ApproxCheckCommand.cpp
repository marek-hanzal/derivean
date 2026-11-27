#include "Arguments.hpp"
#include "core/fn/Approx.hpp"
#include "core/fn/ApproxAngle.hpp"
#include "core/fn/Exact.hpp"
#include "core/fn/Hash.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <thread>
#include <vector>

// `terra approx-check [--only name] [--pairs n]`: every float argument but NaN (both zeros among
// them) of each one-argument float function of core/fn/Approx.hpp and ApproxAngle.hpp against
// CORE-MATH, for the bound of one ulp, for zeros and infinities of the right sign where the exact
// result has them, and for monotonicity between neighbouring floats; pow and atan2 on n random
// pairs (2^32 by default). All cores; minutes on the heavy machine. Exits 1 when a bound or
// property fails.

namespace {

using namespace derivean::sim;

// Floats in order of value, −0 just below +0, and back.
int64_t order(float v) {
    const int32_t i = std::bit_cast<int32_t>(v);
    return i < 0 ? -int64_t(i & INT32_MAX) - 1 : int64_t(i);
}
float unorder(int64_t o) { return std::bit_cast<float>(o >= 0 ? uint32_t(o) : uint32_t(-(o + 1)) | 0x80000000u); }

bool normal(float v) { return std::isfinite(v) && std::abs(v) >= 0x1p-126f; }

struct Tally {
    uint64_t checked = 0, over = 0, reversed = 0, nanMismatch = 0, endMismatch = 0;
    int64_t worst = 0;
    float worstAt = 0, worstAtSecond = 0;
    void merge(const Tally& o) {
        checked += o.checked, over += o.over, reversed += o.reversed, nanMismatch += o.nanMismatch, endMismatch += o.endMismatch;
        if (o.worst > worst) worst = o.worst, worstAt = o.worstAt, worstAtSecond = o.worstAtSecond;
    }
};

// Where the exact result is not normal: ±∞ itself, for ±0 a zero or subnormal of its sign, and for
// a subnormal its sign and the ulp bound.
void compare(Tally& t, float x, float y, float got, float want) {
    ++t.checked;
    if (want != want || got != got) {
        if ((want != want) != (got != got)) ++t.nanMismatch;
        return;
    }
    if (!normal(want)) {
        const bool kept = std::signbit(got) == std::signbit(want) &&
                          (std::isinf(want) ? got == want : std::isfinite(got) && (want != 0 || !normal(got)));
        if (!kept) ++t.endMismatch;
        if (want == 0 || std::isinf(want)) return;
    }
    const int64_t d = std::abs(order(got) - order(want));
    if (d > 1) ++t.over;
    if (d > t.worst) t.worst = d, t.worstAt = x, t.worstAtSecond = y;
}

template<typename F> Tally runAll(F f, int monotone) {
    const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<Tally> parts(threads);
    std::vector<std::thread> pool;
    // Each thread takes a run of the floats in order of value, so neighbours meet in one thread.
    const int64_t first = order(-std::numeric_limits<float>::infinity()), last = order(std::numeric_limits<float>::infinity());
    const int64_t span = (last - first + threads) / threads;
    for (unsigned p = 0; p < threads; ++p) {
        pool.emplace_back([&, p] {
            Tally& t = parts[p];
            const int64_t from = first + int64_t(p) * span, to = std::min(last, from + span);
            float before = std::numeric_limits<float>::quiet_NaN();
            for (int64_t o = from; o <= to; ++o) {
                const float x = unorder(o);
                const auto [got, want] = f(x);
                compare(t, x, 0, got, want);
                if (monotone && got == got && before == before && (monotone > 0 ? got < before : got > before)) ++t.reversed;
                before = got;
            }
        });
    }
    for (auto& thread : pool) thread.join();
    Tally all;
    for (const Tally& t : parts) all.merge(t);
    return all;
}

template<typename F> Tally runPairs(F f, uint64_t pairs, uint64_t salt) {
    const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<Tally> parts(threads);
    std::vector<std::thread> pool;
    for (unsigned p = 0; p < threads; ++p) {
        pool.emplace_back([&, p] {
            for (uint64_t i = p; i < pairs; i += threads) {
                const uint64_t bits = hash(salt, int64_t(i));
                const float a = std::bit_cast<float>(uint32_t(bits)), b = std::bit_cast<float>(uint32_t(bits >> 32));
                const auto [got, want] = f(a, b);
                compare(parts[p], a, b, got, want);
            }
        });
    }
    for (auto& thread : pool) thread.join();
    Tally all;
    for (const Tally& t : parts) all.merge(t);
    return all;
}

bool report(const char* name, const Tally& t, bool pairs) {
    const bool fine = t.over == 0 && t.reversed == 0 && t.nanMismatch == 0 && t.endMismatch == 0;
    char at[96];
    if (pairs) std::snprintf(at, sizeof at, "(%a, %a)", double(t.worstAt), double(t.worstAtSecond));
    else std::snprintf(at, sizeof at, "%a", double(t.worstAt));
    std::printf("%-8s checked %llu  worst %lld ulp at %s  over 1 ulp %llu  reversed %llu  NaN mismatch %llu  0/inf mismatch %llu  %s\n", name,
                (unsigned long long)t.checked, (long long)t.worst, at, (unsigned long long)t.over, (unsigned long long)t.reversed,
                (unsigned long long)t.nanMismatch, (unsigned long long)t.endMismatch, fine ? "ok" : "FAIL");
    std::fflush(stdout);
    return fine;
}

} // namespace

int approxCheckCommand(const Arguments& arguments) {
    const std::string only = arguments.text("only", "");
    const uint64_t pairs = uint64_t(arguments.number("pairs", 4294967296.0));
    bool fine = true;
    const auto one = [&](const char* name, int monotone, auto approximate, auto exactly) {
        if (!only.empty() && only != name) return;
        fine = report(name, runAll([&](float x) { return std::pair{approximate(x), exactly(x)}; }, monotone), false) && fine;
    };
    one("exp", +1, [](float x) { return approx::exp(x); }, [](float x) { return exact::exp(x); });
    one("expm1", +1, [](float x) { return approx::expm1(x); }, [](float x) { return exact::expm1(x); });
    one("log", +1, [](float x) { return approx::log(x); }, [](float x) { return exact::log(x); });
    one("log2", +1, [](float x) { return approx::log2(x); }, [](float x) { return exact::log2(x); });
    one("tanh", +1, [](float x) { return approx::tanh(x); }, [](float x) { return exact::tanh(x); });
    one("atanh", +1, [](float x) { return approx::atanh(x); }, [](float x) { return exact::atanh(x); });
    one("cbrt", +1, [](float x) { return approx::cbrt(x); }, [](float x) { return exact::cbrt(x); });
    one("sin", 0, [](float x) { return approx::sin(x); }, [](float x) { return exact::sin(x); });
    one("cos", 0, [](float x) { return approx::cos(x); }, [](float x) { return exact::cos(x); });
    one("acos", -1, [](float x) { return approx::acos(x); }, [](float x) { return exact::acos(x); });
    if (only.empty() || only == "pow")
        fine = report("pow", runPairs([](float x, float y) { return std::pair{approx::pow(x, y), exact::pow(x, y)}; }, pairs, 1), true) && fine;
    if (only.empty() || only == "atan2")
        fine = report("atan2", runPairs([](float y, float x) { return std::pair{approx::atan2(y, x), exact::atan2(y, x)}; }, pairs, 2), true) && fine;
    return fine ? 0 : 1;
}
