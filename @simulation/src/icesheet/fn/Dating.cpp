#include "icesheet/fn/Dating.hpp"

#include "chronology/constant/Timeline.hpp"
#include "climate/type/Climate.hpp"

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

std::vector<float> stageShares(const ContinentalIce& ice, uint64_t seed, double time) {
    std::vector<float> shares(ice.stages.size(), 0.0f);
    const double end = cycleEnd(time), base = interglacialCooling(seed, time, ice.present);
    for (int k = 0; k < Samples; ++k) {
        const double cooling = base + offsetAt(seed, end, (k + 0.5) / Samples);
        size_t colder = 1;
        while (colder + 1 < ice.stages.size() && ice.stages[colder].cooling > cooling) ++colder;
        const IceStage& warm = ice.stages[colder - 1];
        const IceStage& cold = ice.stages[colder];
        const double towards = warm.cooling > cold.cooling ? std::clamp((warm.cooling - cooling) / (warm.cooling - cold.cooling), 0.0, 1.0) : 0.0;
        shares[colder - 1] += float((1.0 - towards) / Samples);
        shares[colder] += float(towards / Samples);
    }
    return shares;
}

} // namespace derivean::sim
