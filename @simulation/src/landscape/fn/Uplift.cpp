#include "landscape/fn/Build.hpp"

#include "climate/type/Climate.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

constexpr int SinkRadius = 14;          // cells; two passes reach 28, inside the halo, so neighbours agree on the result
constexpr float AscentToDouble = 0.3f;  // m/s of forced ascent that doubles the rain
constexpr int TrendRadius = 12;         // cells over which a finer level is compared with its parent's surface
constexpr float TrendGain = 0.5f;       // share of a drift from that surface taken back in one step
constexpr float TrendLearn = 0.12f;     // share of it by which the sustained uplift is corrected each step

// Two box passes: a cheap bell-shaped average with clamped edges.
void blur(std::vector<float>& field, std::vector<float>& work, int size, int radius) {
    const float scale = 1.0f / float(2 * radius + 1);
    for (int pass = 0; pass < 2; ++pass) {
        for (int z = 0; z < size; ++z) {
            float sum = 0;
            const auto at = [&](int x) { return field[size_t(z) * size_t(size) + size_t(std::clamp(x, 0, size - 1))]; };
            for (int x = -radius; x <= radius; ++x) sum += at(x);
            for (int x = 0; x < size; ++x) {
                work[size_t(z) * size_t(size) + size_t(x)] = sum * scale;
                sum += at(x + radius + 1) - at(x - radius);
            }
        }
        for (int x = 0; x < size; ++x) {
            float sum = 0;
            const auto at = [&](int z) { return work[size_t(std::clamp(z, 0, size - 1)) * size_t(size) + size_t(x)]; };
            for (int z = -radius; z <= radius; ++z) sum += at(z);
            for (int z = 0; z < size; ++z) {
                field[size_t(z) * size_t(size) + size_t(x)] = sum * scale;
                sum += at(z + radius + 1) - at(z - radius);
            }
        }
    }
}

} // namespace

void readClimate(Domain& domain, const Setting& setting) {
    // Only a region lives through dated climate; finer levels settle under the present one.
    const float shift = domain.key.level != Level::Region ? 0.0f
        : float(glacialOffset(setting.seed, domain.time) - glacialOffset(setting.seed, setting.planet->time));
    const float sea = float(domain.seaLevel);
    const int size = domain.size;
    const bool region = domain.key.level == Level::Region;
    for (size_t i = 0; i < domain.cells(); ++i) domain.surface[i] = domain.ground.rock[i] + domain.sedimentAt(i);
    if (region) {
        // Air forced up a slope rains more, air sinking behind it less. No renormalisation: that
        // would tie the rain of a cell to the outline of the tile it happens to be computed in.
        for (int z = 0; z < size; ++z) {
            for (int x = 0; x < size; ++x) {
                const size_t i = domain.index(x, z);
                const auto ground = [&](int ox, int oz) {
                    return std::max(sea, domain.surface[domain.index(std::clamp(x + ox, 0, size - 1), std::clamp(z + oz, 0, size - 1))]);
                };
                const float ascent = float((domain.windX[i] * (ground(1, 0) - ground(-1, 0)) + domain.windZ[i] * (ground(0, 1) - ground(0, -1))) / (2 * domain.cell));
                domain.rain[i] = std::min(4.5f, domain.rainBase[i] * (1.0f + std::clamp(ascent / AscentToDouble, -0.5f, 0.9f)));
            }
        }
    }
    for (size_t i = 0; i < domain.cells(); ++i) {
        domain.warmthNow[i] = domain.warmth[i] + shift - float(LapseRate) * std::max(0.0f, domain.surface[i] - sea);
        domain.runoff[i] = runoffOf(domain.rain[i], domain.warmthNow[i]);
    }
}

void raise(Domain& domain, FlowScratch& scratch) {
    const float dt = float(domain.step);
    const float area = float(domain.cell * domain.cell);
    const bool region = domain.key.level == Level::Region;
    if (region) {
        scratch.blurA.resize(domain.cells());
        scratch.blurB.resize(domain.cells());
        for (size_t i = 0; i < domain.cells(); ++i) scratch.blurA[i] = domain.erosionRate[i] - domain.depositionRate[i];
        blur(scratch.blurA, scratch.blurB, domain.size, SinkRadius);
        for (size_t i = 0; i < domain.cells(); ++i) domain.rebound[i] = float(IsostaticSink) * scratch.blurA[i];
    } else {
        // A finer level adds relief to its parent; it must not move the ground as a whole. Its rock
        // uplift is therefore not given but found: the rate that holds the surface, averaged over
        // about a parent cell, on the parent's surface against the erosion of the finer level.
        // High ground far above its outlet needs a fast rise and gets steep, dissected relief; ground
        // near its base level needs almost none and stays gentle. The parent's own rate is the start.
        scratch.blurA.resize(domain.cells());
        scratch.blurB.resize(domain.cells());
        for (size_t i = 0; i < domain.cells(); ++i) scratch.blurA[i] = domain.trend[i] - domain.ground.rock[i] - domain.sedimentAt(i);
        blur(scratch.blurA, scratch.blurB, domain.size, TrendRadius);
        for (size_t i = 0; i < domain.cells(); ++i) {
            domain.tectonic[i] += TrendLearn * scratch.blurA[i] / dt;
            domain.rebound[i] = TrendGain * scratch.blurA[i] / dt;
        }
    }
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] == Settled) continue;
            const bool pinned = domain.hold[i] == Rim && domain.rimSlope[i] == 0;
            // A ground tile's rock does not sink to make room: what is laid on it stands above the
            // parent's surface until the water takes it on.
            const float found = pinned ? 0.0f : domain.tectonic[i] + domain.rebound[i] + domain.residual[i];
            const float rate = domain.key.level == Level::Ground ? std::max(0.0f, found) : found;
            domain.uplift[i] = rate;
            domain.ground.rock[i] += rate * dt;
            // Only a region's uplift is the motion of real rock that carries its layering up. The
            // rise a finer level finds for itself stands for rock passing through a landscape in
            // balance; there the layering stays where the region left it and relief is cut into it.
            if (region) domain.ground.datum[i] += rate * dt;
            domain.uplifted[i] += rate * dt;
            if (domain.owned(x, z)) domain.ledger.uplifted += double(rate * dt) * area;
        }
    }
}

} // namespace derivean::sim
