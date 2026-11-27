#include "coast/fn/Coast.hpp"
#include "hydrology/constant/Water.hpp"

#include <bit>
#include <numbers>

namespace derivean::sim {
namespace {

constexpr float Reformed = 0.5f;
// A NaN no arithmetic yields (its quiet bit is clear), so no still water compares equal to it.
constexpr float Unraised = std::bit_cast<float>(0x7fa5a5a5u);   // share of the energy a depth holds below which a train is no longer breaking

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
    // 1/tanh is at least 1 and so is its power, and a rounded product does not fall below its
    // factor: kh is never below the deep-water k0·h, so deep water is answered before any of it.
    // Checked over every positive float on macOS libm; it needs tanh ≤ 1 and a monotone pow.
    if (deep > 10) return 0;
    const float kh = deep * std::pow(1.0f / std::tanh(std::pow(deep, 0.75f)), 2.0f / 3.0f);
    if (kh > 10) return 0;
    const float rising = std::tanh(kh), orbit = std::sqrt(kh * rising) / std::sinh(kh), share = 0.5f * (1 + 2 * kh / std::sinh(2 * kh));
    return orbit * orbit * orbit / (share * std::sqrt(rising / kh));
}

// One train at one cell: it arrives as the blend of the three cells behind, grows by the wind over
// the width of the cell, and is cut down by the bed where the water is shallow at high water.
void raiseAt(const Domain& domain, Shore& shore, size_t i, size_t at, int k, bool rim, bool open, float run, ptrdiff_t back, ptrdiff_t backLeft, ptrdiff_t backRight) {
    shore.wave[at] = shore.arrive[at] = shore.crest[at] = 0;
    const float water = shore.still[i];
    if (water == NoWater) return;
    float energy, crest;
    if (rim) {
        energy = crest = open ? shore.beyond[at] : 0.0f;
    } else {
        const auto from = [&](const std::vector<float>& field, ptrdiff_t offset) { return field[size_t(ptrdiff_t(at) + offset)]; };
        const auto blend = [&](const std::vector<float>& field) { return (1.0f - 2.0f * WaveSpread) * from(field, back) + WaveSpread * (from(field, backLeft) + from(field, backRight)); };
        energy = blend(shore.wave), crest = blend(shore.crest);
    }
    shore.arrive[at] = energy;
    // Ground that stands out of high water is still crossed by a train whose swash runs over it,
    // taken as the run-up of what is left of the broken train on the beach face of its breakers.
    // A calibration, not a bore law: it lies between no overwash and the breakers' own run-up
    // (where the berm stands, Drift.cpp), which would sweep every low bar; real storms
    // overwash crests lower than that run-up (Sallenger 2000).
    float depth = water + swingOf(domain, water) - shore.top[i];
    if (depth <= 0 && energy > 0) depth += runUp(std::sqrt(energy), faceSlope(Sand, std::sqrt(crest)));
    if (depth <= 0) return;
    const float gale = shore.gale[at], limit = std::sqrt(shallowSea(depth, gale)), most = std::min(FullSea * gale, limit);
    if (energy < most * most) {
        // Along the curve of this depth, from the deep-water height that stands for the fetch behind.
        // Nothing arriving stands for no fetch: atanh(0) is 0, and limit is above 0 here.
        const float fetched = energy == 0 ? 0.0f : limit * std::atanh(std::min(std::sqrt(energy) / limit, 0.999f));
        const float grown = std::min(most, limit * std::tanh(std::sqrt(fetched * fetched + FetchGrowth * gale * run) / limit));
        energy = std::max(energy, grown * grown);
    } else {
        // A sea higher than this water holds under this wind came from deeper water: the bed wears it down towards that height.
        const float rough = shore.top[i] - domain.ground.rock[i] > LooseBed ? SmoothBed : 1.0f;
        const float height = std::sqrt(energy), friction = rough * bedShare(height, depth) * BedFriction;
        energy = std::max(most, height / (1.0f + friction * height * run / (depth * depth)));
        energy *= energy;
    }
    // A train well below what the depth holds is its own breaker height from here on: in the
    // trough behind a bar or a spit the waves form again and break at their own height, and
    // a swell worn down over a shallow flat breaks at what is left of it. One the depth
    // breaks, or that runs on at the height a flat holds after breaking, keeps the height it
    // reached the first breaking cell with.
    const float holds = Breaker * Breaker * depth * depth;
    shore.crest[at] = energy < Reformed * holds ? energy : std::max(crest, energy);
    shore.wave[at] = std::min(energy, holds);
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
    const size_t cells = domain.cells();
    const bool open = !shore.beyond.empty();
    // Within a step a train at a cell depends on the cell's still water, top and rock and on the
    // trains of the three cells behind it; the wind, the open rim and the holds stay as they are.
    // A cell where none of those moved since the last raise keeps its trains: worked out again they
    // would come out the same to the bit. Inputs are compared bit for bit.
    WaveMemo& memo = shore.raised;
    const bool again = memo.valid && memo.domain == &domain && memo.seaLevel == domain.seaLevel && memo.still.size() == cells;
    if (memo.still.size() != cells) {
        for (auto* field : {&memo.still, &memo.top, &memo.rock}) field->assign(cells, 0.0f);
        memo.changed.assign(cells, 1), memo.moved.assign(cells, 0);
    }
    // A raise over every cell remembers the box alone: a cell outside it is worked out again
    // whenever it joins the box, however its water and ground compare with an earlier step's.
    if (!again) std::fill(memo.still.begin(), memo.still.end(), Unraised);
    const auto same = [](float a, float b) { return std::bit_cast<uint32_t>(a) == std::bit_cast<uint32_t>(b); };
    for (int z = shore.minZ; z <= shore.maxZ; ++z)
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            memo.changed[i] = !again || !same(memo.still[i], shore.still[i]) || !same(memo.top[i], shore.top[i]) || !same(memo.rock[i], domain.ground.rock[i]);
            memo.still[i] = shore.still[i], memo.top[i] = shore.top[i], memo.rock[i] = domain.ground.rock[i];
        }
    for (int k = 0; k < WaveDirections; ++k) {
        const float run = float(domain.cell) * NeighborReach[k];
        const int left = (k + 1) & 7, right = (k + 7) & 7;
        const ptrdiff_t back = -(ptrdiff_t(NeighborZ[k]) * size + NeighborX[k]);
        const ptrdiff_t backLeft = -(ptrdiff_t(NeighborZ[left]) * size + NeighborX[left]), backRight = -(ptrdiff_t(NeighborZ[right]) * size + NeighborX[right]);
        // A cell outside the box may keep a mark from an earlier step: it only costs a cell worked out again.
        for (int z = shore.minZ; z <= shore.maxZ; ++z) std::fill_n(memo.moved.begin() + ptrdiff_t(domain.index(shore.minX, z)), shore.maxX - shore.minX + 1, uint8_t(0));
        sweepDownwind(shore, k, [&](int x, int z) {
            const size_t i = domain.index(x, z), at = trainIndex(cells, i, k);
            const bool rim = domain.hold[i] == Rim;
            if (!memo.changed[i] && (rim || !(memo.moved[size_t(ptrdiff_t(i) + back)] | memo.moved[size_t(ptrdiff_t(i) + backLeft)] | memo.moved[size_t(ptrdiff_t(i) + backRight)])))
                return;
            const float wasWave = shore.wave[at], wasCrest = shore.crest[at];
            raiseAt(domain, shore, i, at, k, rim, open, run, back, backLeft, backRight);
            if (!same(wasWave, shore.wave[at]) || !same(wasCrest, shore.crest[at])) memo.moved[i] = 1;
        });
    }
    memo.valid = true, memo.domain = &domain, memo.seaLevel = domain.seaLevel;
}

} // namespace derivean::sim
