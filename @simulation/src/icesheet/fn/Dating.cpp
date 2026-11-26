#include "icesheet/fn/Dating.hpp"

#include "chronology/constant/Timeline.hpp"
#include "climate/type/Climate.hpp"

#include "core/type/Vec.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr int Samples = 50;   // points of a cycle its climate is read at

// The sawtooth of `glacialOffset` at a phase of the cycle that ends at `end`.
double offsetAt(uint64_t seed, double end, double phase) { return glacialOffset(seed, end - GlacialCycle * (1.0 - phase)); }

double cycleEnd(double time) { return std::floor(time / GlacialCycle) * GlacialCycle; }

} // namespace

double lastTermination(double age) { return cycleEnd(age); }

double interglacialCooling(uint64_t seed, double time, double present) {
    return std::min(0.0, greenhouseOffset(seed, cycleEnd(time)) - greenhouseOffset(seed, present));
}

double maximumCooling(uint64_t seed, double time, double present) {
    const double end = cycleEnd(time);
    double deepest = 0;
    for (int k = 0; k < Samples; ++k) deepest = std::min(deepest, offsetAt(seed, end, (k + 0.5) / Samples));
    return interglacialCooling(seed, time, present) + deepest;
}

std::vector<float> stageShares(const std::vector<double>& stage, uint64_t seed, double time, double present) {
    std::vector<float> shares(stage.size(), 0.0f);
    if (stage.size() < 2) {
        if (!shares.empty()) shares[0] = 1;
        return shares;
    }
    const double end = cycleEnd(time), base = interglacialCooling(seed, time, present);
    for (int k = 0; k < Samples; ++k) {
        const double cooling = base + offsetAt(seed, end, (k + 0.5) / Samples);
        size_t colder = 1;
        while (colder + 1 < stage.size() && stage[colder] > cooling) ++colder;
        const double warm = stage[colder - 1], cold = stage[colder];
        const double towards = warm > cold ? std::clamp((warm - cooling) / (warm - cold), 0.0, 1.0) : 0.0;
        shares[colder - 1] += float((1.0 - towards) / Samples);
        shares[colder] += float(towards / Samples);
    }
    return shares;
}

double sinceMaximum(uint64_t seed, double present) {
    const double end = cycleEnd(present);
    double deepest = 0, phase = 1;
    for (int k = 0; k < Samples; ++k)
        if (const double offset = offsetAt(seed, end, (k + 0.5) / Samples); offset < deepest) deepest = offset, phase = (k + 0.5) / Samples;
    return GlacialCycle * (1.0 - phase);
}

} // namespace derivean::sim
