#include "landscape/fn/Process.hpp"

#include "climate/fn/Seasons.hpp"
#include "landscape/fn/Strength.hpp"

namespace derivean::sim {
namespace {

constexpr float MantleShare = 0.1f;      // share of a cell's width in loose cover (at least a metre) above which the rock under it is not the face that stands too steep
constexpr float OpenCover = 0.3f;        // m of loose cover under which a face is open to the weather
constexpr float RestingTan = 0.8f;       // rise over run up to which a mantle of debris stays where it is
constexpr float RampTan = 0.75f;         // rise over run of the rock ramp a retreating face leaves at its foot
constexpr float FreeFace = 1.19f;        // rise over run (50°) above which debris does not rest on rock
constexpr float CrushedLoss = 0.85f;     // share of its cohesion that fully crushed rock has lost
constexpr float BrokenRetreat = 3.0f;    // gain of a face's retreat in fully broken rock
constexpr float CoarseFlank = 0.45f;      // share of a face's stable drop that the mean surface of a kilometre-wide cell holds
constexpr double NominalHalfRange = 10;  // °C; the frost a level sees is that of this seasonal swing at the cell's own mean

bool underWater(const Domain& domain, size_t i) { return domain.level[i] > domain.surface[i] + 1e-3f; }

// What stays fixed for a replay: how sound the rock of each cell is against its material's own strength.
void measureSoundness(const Domain& domain, FlowScratch& scratch) {
    if (scratch.soundScale.size() == domain.cells()) return;
    scratch.soundScale.resize(domain.cells());
    scratch.soundLog.resize(domain.cells());
    for (size_t i = 0; i < domain.cells(); ++i) {
        const float factor = std::clamp(domain.sound[i] * (1.0f - CrushedLoss * domain.weakness[i]) / std::max(0.05f, domain.quality[i]), 0.05f, 4.0f);
        scratch.soundScale[i] = std::pow(factor, 0.75f);
        scratch.soundLog[i] = std::log2(scratch.soundScale[i]);
    }
}

// A free face is weathered sideways: it retreats by what the weather takes off bare rock. The part
// of the face below the column's own bed is the rock that crops out at its foot, so a weak bed
// under a strong one undermines it at the weak bed's pace. A height field cannot hold a face that
// has moved by part of a cell, so each column keeps how far its face has gone, and when that is the
// width of the column it comes down at once: the wall steps back one cell and stays a wall, leaving
// under it the rock ramp that retreating cliffs leave, at about the angle their debris lies at.
void retreatFaces(Domain& domain, FlowScratch& scratch) {
    const int size = domain.size;
    const float cell = float(domain.cell), dt = float(domain.step), area = cell * cell;
    if (scratch.retreated.size() != domain.cells()) scratch.retreated.assign(domain.cells(), 0.0f);
    for (int z = 1; z < size - 1; ++z) {
        for (int x = 1; x < size - 1; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free || domain.sedimentAt(i) > OpenCover || underWater(domain, i)) continue;
            const float top = domain.ground.rock[i];
            int best = -1;
            float steepest = FreeFace;
            for (int k = 0; k < 8; ++k) {
                const float fall = (top - scratch.work[domain.index(x + NeighborX[k], z + NeighborZ[k])]) / (NeighborReach[k] * cell);
                if (fall > steepest) steepest = fall, best = k;
            }
            if (best < 0) {
                scratch.retreated[i] = 0;
                continue;
            }
            const size_t foot = domain.index(x + NeighborX[best], z + NeighborZ[best]);
            const float face = top - scratch.work[foot];
            const UnitCache& bed = domain.unit[i];
            const float own = std::clamp(bed.bottom - std::max(0.0f, domain.ground.datum[i] - top), 0.0f, face);
            // Broken rock comes off a face faster than sound rock.
            const float wet = (0.3f + 0.7f * std::min(1.0f, domain.rain[i])) * (1.0f + DebrisFrost * frostShare(domain, i)) * (1.0f + BrokenRetreat * domain.weakness[i]);
            const float run = NeighborReach[best] * cell;
            scratch.retreated[i] += wet * dt * (bed.traits.weathering * own + domain.unit[foot].traits.weathering * (face - own)) / face;
            if (scratch.retreated[i] < run) continue;
            scratch.retreated[i] -= run;
            const float lowered = face - RampTan * run;
            const RockTraits& rock = bed.traits;
            domain.ground.rock[i] -= lowered;
            domain.ground.sediment[Coarse][i] += lowered * rock.coarse;
            domain.ground.sediment[Sand][i] += lowered * rock.sand;
            domain.ground.sediment[Fines][i] += lowered * rock.fines;
            scratch.work[i] -= lowered * rock.solute;
            scratch.broke[i] += lowered;
            if (!domain.owned(x, z)) continue;
            domain.ledger.failed += double(lowered) * area;
            domain.ledger.dissolved += double(lowered * rock.solute) * area;
        }
    }
}

// Lowest cells first, so a cell is judged against ground that has already found its height. Every
// cell carries the foot of the slope it stands on: the lowest point from which the ground has risen
// without easing below the rock's friction angle. A slope of height H above its foot holds a mean
// angle that falls as H grows (landscape/fn/Strength.hpp): a cell may stand no higher above that
// foot than its own rock holds over the straight line between them, and no higher above any lower
// neighbour than a fresh face of that run. Straight-line distances keep faces true to the valley
// they fall to instead of to the grid. What breaks is gone from the face for the cells above; it
// lies on the cell as debris until settleDebris moves it, or leaves with the cell's water.
void failFaces(Domain& domain, FlowScratch& scratch) {
    const int size = domain.size;
    const float cell = float(domain.cell), area = cell * cell, logCell = std::log2(cell);
    const float mantleLimit = std::max(1.0f, MantleShare * cell);
    // A cell far wider than a rock face holds the mean of a dissected flank: walls, the benches
    // between them and the gullies that cut both. That mean stands at a share of what a face holds.
    const float flank = float(1.0 - (1.0 - CoarseFlank) * smoothstep(100.0, 1000.0, domain.cell));
    const bool delivers = !scratch.delivered[0].empty();
    std::vector<float>& height = scratch.work;
    std::vector<int32_t>& foot = scratch.foot;
    foot.resize(domain.cells());
    for (size_t i = 0; i < foot.size(); ++i) foot[i] = int32_t(i);   // a neighbour of equal height may not have been visited yet
    const auto runTo = [&](int x, int z, int32_t at) {
        const int dx = x - at % size, dz = z - at / size;
        return std::sqrt(float(dx * dx + dz * dz));
    };
    for (const int32_t at : scratch.byHeight) {
        const size_t i = size_t(at);
        const int x = at % size, z = at / size;
        foot[i] = at;
        if (domain.hold[i] != Free || x < 1 || z < 1 || x >= size - 1 || z >= size - 1) continue;
        const float here = height[i];
        const StrengthCurve& curve = strengthOf(domain.unit[i].traits.dominant);
        float steepest = 0;
        for (int k = 0; k < 8; ++k) steepest = std::max(steepest, (here - height[domain.index(x + NeighborX[k], z + NeighborZ[k])]) / NeighborReach[k]);
        // Ground gentler than the rock's friction angle is no part of a slope that can fail: a foot of its own.
        const float friction = flank * curve.frictionTan * cell;
        if (steepest < 0.9f * friction) continue;

        const float scale = scratch.soundScale[i], logScale = scratch.soundLog[i];
        const auto hold = [&](float run) { return run <= 0 ? 0.0f : flank * stableDrop(curve, logCell + std::log2(run), scale, logScale); };
        // What a face shows below the cell's own bed is the rock at its foot. A face from `low` that
        // stays within the bed holds what the bed's rock holds; one that reaches below it is two
        // faces stacked: the rock under the bed up to what that rock holds, and on it the bed, up to
        // its own thickness or what its rock holds. A thin weak bed behind a strong lip thus keeps the
        // lip, and a thin strong cap rides on what the weak rock under it holds.
        const UnitCache& bed = domain.unit[i];
        const float own = std::max(0.0f, bed.bottom - std::max(0.0f, domain.ground.datum[i] - domain.ground.rock[i]));
        const float bedBase = domain.ground.rock[i] - own;
        const auto stack = [&](float low, float ownHold, float belowHold) {
            return low >= bedBase ? low + ownHold : std::min(bedBase, low + belowHold) + std::min(here - bedBase, ownHold);
        };
        const auto holdOf = [&](size_t other, float run) {
            return run <= 0 ? 0.0f : flank * stableDrop(strengthOf(domain.unit[other].traits.dominant), logCell + std::log2(run), scratch.soundScale[other], scratch.soundLog[other]);
        };
        // Two things hold a cell down: the face it makes with each lower neighbour, as if that
        // neighbour were level ground, and the whole slope from the foot that neighbour stands on.
        float allowed = 1e30f, slope = 1e30f;
        int32_t base = at;
        for (int k = 0; k < 8; ++k) {
            const size_t lower = domain.index(x + NeighborX[k], z + NeighborZ[k]);
            if (height[lower] >= here) continue;
            allowed = std::min(allowed, stack(height[lower], hold(NeighborReach[k]), holdOf(lower, NeighborReach[k])));
            const int32_t from = foot[lower];
            const float run = runTo(x, z, from);
            const float whole = stack(height[size_t(from)], hold(run), holdOf(size_t(from), run));
            if (whole < slope) slope = whole, base = from;
        }
        allowed = std::min(allowed, slope);
        // Loose cover shields the rock under it only where it can lie: on ground steeper than
        // debris rests at it is on its way down, and the rock is the face.
        const float loose = domain.sedimentAt(i);
        const bool mantled = loose > mantleLimit && steepest <= RestingTan * cell;
        if (allowed >= here || mantled) {
            // Standing as it is. Bare rock stays on the slope of that foot while the ground from there
            // up to it is steeper than friction; an apron of debris props what stands above it, and
            // the face over it starts anew at its top.
            if (!mantled && here - height[size_t(base)] >= friction * runTo(x, z, base)) foot[i] = base;
            continue;
        }
        foot[i] = base;
        const float fallen = here - allowed, fromRock = std::max(0.0f, fallen - loose);
        height[i] = allowed;
        if (fromRock <= 0) continue;
        // A fall breaks into the sizes the rock weathers to; what would have gone into solution
        // with time is still rock dust here.
        const RockTraits& rock = domain.unit[i].traits;
        const float broken[SedimentClasses] = {fromRock * rock.coarse, fromRock * rock.sand, fromRock * (rock.fines + rock.solute)};
        domain.ground.rock[i] -= fromRock;
        for (int c = 0; c < SedimentClasses; ++c) (delivers ? scratch.delivered[c][i] : domain.ground.sediment[c][i]) += broken[c];
        scratch.broke[i] += fromRock;
        if (domain.owned(x, z)) domain.ledger.failed += double(fromRock) * area;
    }
}

} // namespace

float frostShare(const Domain& domain, size_t i) {
    if (domain.key.level == Level::Ground) return domain.frost;
    // Annual means from −30 to +20 °C in half degrees.
    static const auto table = [] {
        std::array<float, 101> shares;
        for (size_t k = 0; k < shares.size(); ++k) shares[k] = float(frostCracking(-30.0 + 0.5 * double(k), NominalHalfRange));
        return shares;
    }();
    return table[size_t(std::clamp(int(std::lround((domain.warmthNow[i] + 30.0f) * 2.0f)), 0, 100))];
}

void orderByHeight(const std::vector<float>& height, std::vector<int32_t>& order, std::vector<int32_t>& spare) {
    constexpr int Bits = 11, Buckets = 1 << Bits;
    const size_t cells = height.size();
    float low = height[0], high = height[0];
    for (const float h : height) low = std::min(low, h), high = std::max(high, h);
    const float perMetre = float((1 << (2 * Bits)) - 1) / std::max(1e-3f, high - low);
    order.resize(cells), spare.resize(cells);
    std::vector<uint32_t> key(cells);
    for (size_t i = 0; i < cells; ++i) key[i] = uint32_t((height[i] - low) * perMetre), spare[i] = int32_t(i);
    // Two stable passes of a counting sort: ties stay in index order.
    for (int pass = 0; pass < 2; ++pass) {
        uint32_t count[Buckets + 1] = {};
        const std::vector<int32_t>& from = pass == 0 ? spare : order;
        std::vector<int32_t>& to = pass == 0 ? order : spare;
        for (const int32_t i : from) ++count[((key[size_t(i)] >> (pass * Bits)) & (Buckets - 1)) + 1];
        for (int b = 0; b < Buckets; ++b) count[b + 1] += count[b];
        for (const int32_t i : from) to[count[(key[size_t(i)] >> (pass * Bits)) & (Buckets - 1)]++] = i;
    }
    order.swap(spare);
}

void breakSlopes(Domain& domain, FlowScratch& scratch) {
    const size_t cells = domain.cells();
    measureSoundness(domain, scratch);
    scratch.work.resize(cells);
    for (size_t i = 0; i < cells; ++i) scratch.work[i] = domain.ground.rock[i] + domain.sedimentAt(i);
    scratch.broke.assign(cells, 0.0f);
    // A Region cell is wider than any apron of debris: what breaks off inside it reaches the
    // streams it hides within the step and travels on as their load. Finer levels keep it as talus.
    const bool region = domain.key.level == Level::Region;
    for (auto& layer : scratch.delivered) layer.assign(region ? cells : 0, 0.0f);
    if (!region) retreatFaces(domain, scratch);
    orderByHeight(scratch.work, scratch.byHeight, scratch.spare);
    failFaces(domain, scratch);
    const float dt = float(domain.step);
    for (size_t i = 0; i < cells; ++i) domain.shed[i] += 0.4f * (scratch.broke[i] / dt - domain.shed[i]);
}

} // namespace derivean::sim
