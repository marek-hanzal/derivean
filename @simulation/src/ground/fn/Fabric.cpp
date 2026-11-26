#include "ground/fn/Fabric.hpp"

#include "core/fn/Hash.hpp"
#include "ground/constant/Ground.hpp"
#include "ground/fn/Frost.hpp"
#include "ground/fn/Partings.hpp"

namespace derivean::sim {
namespace {

// Where the main flow of a cell goes, as a heading in radians; false if it has none.
bool flowHeading(const FabricContext& context, size_t i, float& heading, float& fall) {
    const Domain& domain = context.domain;
    const float* share = &context.scratch.share[i * 8];
    int main = -1;
    for (int k = 0; k < 8; ++k)
        if (share[k] > 0 && (main < 0 || share[k] > share[main])) main = k;
    if (main < 0) return false;
    const size_t to = domain.index(int(i % size_t(domain.size)) + NeighborX[main], int(i / size_t(domain.size)) + NeighborZ[main]);
    heading = std::atan2(float(NeighborZ[main]), float(NeighborX[main]));
    fall = std::max(0.0f, domain.level[i] - domain.level[to]) / (float(domain.cell) * NeighborReach[main]);
    return true;
}

} // namespace

FabricContext fabricContextOf(const StoneBed& stones, const FlowScratch& scratch, const TileLookup& lookup) {
    const Domain& domain = stones.domain;
    const uint64_t seed = stones.seed;
    FabricContext context{domain, scratch, lookup, {}, {}, {}, {}};
    // From the rock mass of each cell's bed, asked once per bed and strata node; a domain built by
    // hand of one rock has no beds, and its rock's kind has to do.
    context.habit.resize(domain.cells());
    size_t massNode = SIZE_MAX;
    Bed massBed{0, -1, Material::Bedrock};
    double massLaid = 0;
    BlockHabit known;
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.bed.empty()) {
            context.habit[i] = blockHabitOf(domain.unit[i].traits);
            continue;
        }
        const Bed& bed = domain.bed[i];
        const size_t node = size_t(int(i / size_t(domain.size)) / domain.stride) * size_t(domain.nodes) + size_t(int(i % size_t(domain.size)) / domain.stride);
        if (node != massNode || bed.rock != massBed.rock || canonicalThickness(bed) != massBed.bottom || domain.laidDown[i] != massLaid) {
            massNode = node, massBed = {0, canonicalThickness(bed), bed.rock}, massLaid = domain.laidDown[i];
            known = blockHabitOf(massOf(seed, domain.strata[node], bed, domain.laidDown[i]), traitsOf(bed.rock));
        }
        context.habit[i] = known;
    }
    const int size = domain.size;
    context.slope.assign(domain.cells(), 0.0f);
    for (int z = 2; z < size - 2; ++z)
        for (int x = 2; x < size - 2; ++x) {
            const float dx = stones.groundAt(domain.index(x + 2, z)) - stones.groundAt(domain.index(x - 2, z));
            const float dz = stones.groundAt(domain.index(x, z + 2)) - stones.groundAt(domain.index(x, z - 2));
            context.slope[domain.index(x, z)] = std::sqrt(dx * dx + dz * dz) / float(4 * domain.cell);
        }
    // Waste of a failing face goes down the fall line; upstream cells first, so one sweep carries it all the way.
    context.face.assign(domain.cells(), -1);
    context.fallen.assign(domain.cells(), 0.0f);
    for (size_t step = domain.cells(); step-- > 0;) {
        const size_t i = size_t(domain.order[step]);
        if (!domain.shed.empty() && domain.shed[i] > FailingFace && domain.sedimentAt(i) < 0.5f) context.face[i] = int32_t(i), context.fallen[i] = 0;
        if (context.face[i] < 0) continue;
        const float* share = &scratch.share[i * 8];
        int main = -1;
        for (int k = 0; k < 8; ++k)
            if (share[k] > 0 && (main < 0 || share[k] > share[main])) main = k;
        if (main < 0) continue;
        const size_t to = domain.index(int(i % size_t(size)) + NeighborX[main], int(i / size_t(size)) + NeighborZ[main]);
        const float reached = context.fallen[i] + float(domain.cell) * NeighborReach[main];
        if (context.face[to] < 0 || reached < context.fallen[to]) context.face[to] = context.face[i], context.fallen[to] = reached;
    }
    return context;
}

Fabric fabricAt(const FabricContext& context, size_t i, uint64_t draw) {
    const Domain& domain = context.domain;
    Fabric fabric;
    const float coarse = domain.ground.sediment[Coarse][i], fine = domain.ground.sediment[Sand][i] + domain.ground.sediment[Fines][i];
    if (coarse < 0.01f) return fabric;
    const RockTraits& local = domain.unit[i].traits;
    const int x = int(i % size_t(domain.size)), z = int(i / size_t(domain.size));
    const Vec2 here = domain.center(x, z);
    const auto habitOf = [&](const BlockHabit& habit, const RockTraits& rock) {
        fabric.flatness = habit.flatness;
        fabric.blockShare = rock.coarse > 0 ? std::max(0.0f, 1.0f - definitionOf(rock.dominant).weathersTo.gravel / rock.coarse) : 0.0f;
        return habit.size;
    };
    fabric.rock = local.dominant;
    fabric.source = here;
    const float depth = domain.level[i] - domain.surface[i];
    float heading = 0, fall = 0;
    const bool flows = flowHeading(context, i, heading, fall);

    if (flows && domain.stream[i] >= BedFlow && depth < 0.6f) {
        // The bed of running water: what the flow cannot move stays as its armour. Where each stone
        // came from is found once it is known to lie here.
        const float able = competence(domain.stream[i], std::max(fall, context.slope[i] * 0.5f));
        const float supply = habitOf(context.habit[i], local);
        fabric.origin = ClastOrigin::Stream;
        fabric.median = std::min(1.3f * supply, std::max(0.6f * able, 0.3f * supply * std::min(1.0f, able / 0.05f)));
        fabric.cover = able > 0.06f ? 0.55f : 0.0f;
        fabric.blockShare = 1;
        fabric.sinkLow = 0.3f, fabric.sinkHigh = 0.6f;
        fabric.heading = heading;
        fabric.imbricate = true;
        return fabric;
    }
    if (depth > 0.05f) return fabric;   // under standing water nothing shows
    if (domain.iceAge[i] > LeastIce && coarse > 0.2f && fine > 0.25f * coarse) {
        // Till: whatever the ice carried, unsorted, with rock from up its path among it.
        const IceSource far = iceSourceAt(context.lookup, here);
        const bool erratic = far.rock != Material::Air && far.rock != local.dominant && unit(hash(draw, 3)) < 0.55;
        if (erratic) habitOf(blockHabitOf(traitsOf(far.rock)), traitsOf(far.rock));
        else habitOf(context.habit[i], local);
        fabric.origin = ClastOrigin::Till;
        fabric.median = 0.32f, fabric.spread = 0.85f;
        fabric.cover = 0.12f;
        fabric.blockShare = 0.5f;
        fabric.flatness = std::max(0.6f, fabric.flatness);
        fabric.roundness = 0.4f;
        fabric.sinkLow = 0.25f, fabric.sinkHigh = 0.85f;
        if (erratic) fabric.rock = far.rock, fabric.source = far.source, fabric.travel = float(length(far.source - here));
        return fabric;
    }
    const float share = coarse / (coarse + fine + 0.02f);
    if (frostAt(domain, i) > FrostWork && context.slope[i] < 0.5f && share > 0.5f) {
        // Frost-shattered rock that has not left: a block field, or the stone border of sorted ground.
        fabric.origin = ClastOrigin::Frost;
        fabric.median = 0.7f * habitOf(context.habit[i], local);
        fabric.spread = 0.45f;
        fabric.cover = 0.7f * share;
        fabric.blockShare = std::max(0.6f, fabric.blockShare);
        fabric.roundness = 0.08f;
        fabric.sinkLow = 0.25f, fabric.sinkHigh = 0.5f;
        return fabric;
    }
    if (context.face[i] >= 0 && coarse > 0.12f && context.slope[i] > 0.3f) {
        // Scree: the waste of the face above, coarser the farther it rolled.
        const size_t face = size_t(context.face[i]);
        const RockTraits& rock = domain.unit[face].traits;
        fabric.origin = ClastOrigin::Fall;
        fabric.median = habitOf(context.habit[face], rock) * (0.55f + 0.5f * std::min(1.0f, context.fallen[i] / 16.0f));
        fabric.spread = 0.45f;
        fabric.cover = 0.6f * std::min(1.0f, coarse / 0.4f) * share;
        fabric.blockShare = std::max(0.5f, fabric.blockShare);
        fabric.roundness = 0.08f;
        fabric.rock = rock.dominant;
        fabric.source = domain.center(int(face % size_t(domain.size)), int(face / size_t(domain.size)));
        fabric.travel = context.fallen[i];
        fabric.sinkLow = 0.15f, fabric.sinkHigh = 0.45f;
        return fabric;
    }
    // Lag: the blocks of the rock underneath, left as finer waste went. No block is larger than its
    // joints cut it, and massive rock weathers to rounded cores where bedded rock leaves slabs.
    const float whole = habitOf(context.habit[i], local);
    fabric.median = 0.6f * whole;
    fabric.largest = 1.6f * whole;
    fabric.cover = 0.5f * share;
    fabric.roundness = float(0.15 + 0.5 * (1.0 - local.bedding) + 0.15 * unit(hash(draw, 4)));
    // A stone shows only if it stands through whatever fine cover lies over the coarse layer.
    fabric.buried = fine;
    return fabric;
}

} // namespace derivean::sim
