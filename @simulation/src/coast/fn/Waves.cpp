#include "coast/fn/Coast.hpp"
#include "hydrology/constant/Water.hpp"

#include <numbers>

namespace derivean::sim {
namespace {

constexpr float LooseBed = 0.1f;        // m of loose cover from which a bed is sand or mud to the waves, not rock
constexpr float SmoothBed = 0.2f;       // friction of such a bed against a rock bench's
// Fetch-limited growth over water of any depth (Shore Protection Manual 1984, eq. 3-39):
// g·H/U² = 0.283·tanh(0.530·(g·d/U²)^¾)·tanh(0.00565·(g·F/U²)^½ / tanh(0.530·(g·d/U²)^¾)),
// i.e. H = L·tanh(H_deep/L) with L the highest sea the depth holds under that wind, where what the
// wind gives and what the bed takes balance, and H_deep the deep-water growth of FetchGrowth
// (0.283 × 0.00565 = 0.0016). In deep water L is 0.283·U²/g, above the fully developed sea FullSea·U²
// that caps every train.
constexpr float ShallowSea = 0.283f;
constexpr float ShallowDepth = 0.530f;

// m², the square of the highest wave a wind of square speed `gale` raises over water `depth` deep.
float shallowSea(float depth, float gale) {
    const float g = float(Gravity), height = ShallowSea * std::tanh(ShallowDepth * std::pow(g * depth / gale, 0.75f)) * gale / g;
    return height * height;
}

// Share of the shallow-water friction a wind sea of `height` meets over a bed `depth` below: by
// linear theory the motion at the bed is the shallow-water one times √(kh·tanh kh)/sinh kh, and the
// energy travels at n·c instead of √(g·h). kh by Fenton and McKee (1990) from the deep-water k0·h.
float bedShare(float height, float depth) {
    const float deep = 2 * float(std::numbers::pi) * WindSeaSteepness * depth / height;
    const float kh = deep * std::pow(1.0f / std::tanh(std::pow(deep, 0.75f)), 2.0f / 3.0f);
    if (kh > 10) return 0;
    const float rising = std::tanh(kh), orbit = std::sqrt(kh * rising) / std::sinh(kh), share = 0.5f * (1 + 2 * kh / std::sinh(2 * kh));
    return orbit * orbit * orbit / (share * std::sqrt(rising / kh));
}

} // namespace

float stormWind(const Domain& domain, size_t cell, int train) {
    const float wx = domain.windX[cell], wz = domain.windZ[cell];
    const float speed = std::sqrt(wx * wx + wz * wz);
    const float along = speed > 1e-3f ? std::max(0.0f, (wx * NeighborX[train] + wz * NeighborZ[train]) / (speed * NeighborReach[train])) : 0.0f;
    return StormGain * (speed * along * along + StormCalm);
}

// Each train is carried from cell to cell in the direction it travels: it arrives as the blend of
// the three cells behind, grows by the wind over the width of the cell, and is cut down by the
// bed where the water is shallow at high water. A cell neither the water nor the train's own
// swash covers lets nothing through, so land shelters what lies behind it; the blend lets a train
// turn into that shelter by a little with every cell, as waves do around a point.
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
            // Ground that stands out of high water is still crossed by a train whose swash runs over it.
            float depth = water + swingOf(domain, water) - shore.top[i];
            if (depth <= 0 && energy > 0) depth += runUp(std::sqrt(energy), faceSlope(Sand, std::sqrt(energy)));
            if (depth <= 0) return;
            const float gale = shore.gale[at], limit = std::sqrt(shallowSea(depth, gale)), most = std::min(FullSea * gale, limit);
            if (energy < most * most) {
                // Along the curve of this depth, from the deep-water height that stands for the fetch behind.
                const float fetched = limit * std::atanh(std::min(std::sqrt(energy) / limit, 0.999f));
                const float grown = std::min(most, limit * std::tanh(std::sqrt(fetched * fetched + FetchGrowth * gale * run) / limit));
                energy = std::max(energy, grown * grown);
            } else {
                // A sea higher than this water holds under this wind came from deeper water: the bed wears it down towards that height.
                const float rough = shore.top[i] - domain.ground.rock[i] > LooseBed ? SmoothBed : 1.0f;
                const float height = std::sqrt(energy), friction = rough * bedShare(height, depth) * BedFriction;
                energy = std::max(most, height / (1.0f + friction * height * run / (depth * depth)));
                energy *= energy;
            }
            shore.wave[at] = std::min(energy, Breaker * Breaker * depth * depth);
        });
    }
}

} // namespace derivean::sim
