#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

constexpr float FrictionReach = 5.0f;   // m: depth² under FrictionReach·width is where friction takes more than a hundredth of a wave

} // namespace

float stormWind(const Domain& domain, size_t cell, int train) {
    const float wx = domain.windX[cell], wz = domain.windZ[cell];
    const float speed = std::sqrt(wx * wx + wz * wz);
    const float along = speed > 1e-3f ? std::max(0.0f, (wx * NeighborX[train] + wz * NeighborZ[train]) / (speed * NeighborReach[train])) : 0.0f;
    return StormGain * (speed * along * along + StormCalm);
}

// Each train is carried from cell to cell in the direction it travels: it arrives as the blend of
// the three cells behind, grows by the wind over the width of the cell, and is cut down by the
// bed where the water is shallow at high water. A cell the water does not cover lets nothing
// through, so land shelters what lies behind it; the blend lets a train turn into that shelter by
// a little with every cell, as waves do around a point.
void raiseWaves(const Domain& domain, Shore& shore) {
    const int size = domain.size;
    const bool open = !shore.beyond.empty();
    for (int k = 0; k < WaveDirections; ++k) {
        const float run = float(domain.cell) * NeighborReach[k];
        const int left = (k + 1) & 7, right = (k + 7) & 7;
        const ptrdiff_t back = -(ptrdiff_t(NeighborZ[k]) * size + NeighborX[k]);
        const ptrdiff_t backLeft = -(ptrdiff_t(NeighborZ[left]) * size + NeighborX[left]), backRight = -(ptrdiff_t(NeighborZ[right]) * size + NeighborX[right]);
        sweepDownwind(shore, k, [&](int x, int z) {
            const size_t i = domain.index(x, z), at = i * WaveDirections + size_t(k);
            shore.wave[at] = shore.arrive[at] = 0;
            const float water = shore.still[i];
            if (water == NoWater) return;
            float energy;
            if (domain.hold[i] == Rim) {
                energy = open ? shore.beyond[at] : 0.0f;
            } else {
                const auto from = [&](ptrdiff_t offset) { return shore.wave[size_t(ptrdiff_t(i) + offset) * WaveDirections + size_t(k)]; };
                energy = (1.0f - 2.0f * WaveSpread) * from(back) + WaveSpread * (from(backLeft) + from(backRight));
            }
            shore.arrive[at] = energy;
            const float depth = water + swingOf(domain, water) - shore.top[i];
            if (depth <= 0) return;
            const float gale = shore.gale[at], full = FullSea * FullSea * gale * gale;
            if (energy < full) energy = std::min(full, energy + FetchGrowth * gale * run);
            // The bed is only felt where the water is shallow against the width of the cell.
            if (depth * depth < FrictionReach * run) {
                const float height = std::sqrt(energy), worn = height / (1.0f + BedFriction * height * run / (depth * depth));
                energy = worn * worn;
            }
            shore.wave[at] = std::min(energy, Breaker * Breaker * depth * depth);
        });
    }
}

} // namespace derivean::sim
