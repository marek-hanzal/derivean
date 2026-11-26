#include "landscape/fn/Build.hpp"

#include "climate/type/Climate.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

constexpr int SinkRadius = 14;          // cells; two passes reach 28, inside the halo, so neighbours agree on the result
constexpr float AscentToDouble = 0.3f;  // m/s of forced ascent that doubles the rain
constexpr int TrendRadius = 12;         // cells over which a ground tile is compared with its parent's surface
constexpr float TrendGain = 0.5f;       // share of a drift from that surface taken back in one step
constexpr float TrendLearn = 0.12f;     // share of it by which the sustained uplift is corrected each step
// Parent cells per side of the block over which a Local or Site domain is held to its parent's
// surface. It must be wider than the valleys the level cuts: a lift that answers a narrower
// neighbourhood raises shoulders beside every cut, the shoulder turns the slope's water aside and
// the next valley starts at its foot, so the level ribs itself at the width of its own average.
constexpr int HeldParents = 4;

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

// Replaces `field` by a smooth surface through its means over the evolving cells of blocks of
// `span` cells laid from the domain's corner. Cells a finished neighbour holds and the rim do not
// move with the lift and are left out of the means; a block without evolving cells asks for nothing.
// Between block centres the surface follows a smoothstep, which reads the two blocks on either side
// only: a tile is a whole number of blocks wide and its halo half a block, so the owned cells of a
// tile and of its neighbour read the same blocks and agree along their seam.
void overBlocks(std::vector<float>& field, const std::vector<uint8_t>& hold, int size, int span) {
    const int count = (size + span - 1) / span;
    Field<double> mean(count, count), cells(count, count);
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const size_t i = size_t(z) * size_t(size) + size_t(x);
            if (hold[i] != Free) continue;
            mean.at(x / span, z / span) += field[i], cells.at(x / span, z / span) += 1;
        }
    for (size_t i = 0; i < mean.size(); ++i) mean.cells[i] = cells.cells[i] > 0 ? mean.cells[i] / cells.cells[i] : 0.0;
    // Block below and weight of the block above, for every cell along one axis.
    std::vector<int> below(static_cast<size_t>(size));
    std::vector<double> weight(static_cast<size_t>(size));
    for (int k = 0; k < size; ++k) {
        const double at = std::clamp((k + 0.5) / span - 0.5, 0.0, double(count - 1));
        below[size_t(k)] = std::min(int(at), std::max(0, count - 2));
        weight[size_t(k)] = smoothstep(0.0, 1.0, at - below[size_t(k)]);
    }
    for (int z = 0; z < size; ++z) {
        const int bz = below[size_t(z)], nz = std::min(bz + 1, count - 1);
        const double wz = weight[size_t(z)];
        for (int x = 0; x < size; ++x) {
            const int bx = below[size_t(x)], nx = std::min(bx + 1, count - 1);
            const double wx = weight[size_t(x)];
            const double near = mix(mean.at(bx, bz), mean.at(nx, bz), wx), far = mix(mean.at(bx, nz), mean.at(nx, nz), wx);
            field[size_t(z) * size_t(size) + size_t(x)] = float(mix(near, far, wz));
        }
    }
}

// A block that straddles a finished neighbour is averaged over different cells by the two tiles:
// the earlier one over its own half and its halo, the later one over its own half alone. Each found
// its own rate there, and the seam carried the difference as a step or a gutter (on the humid range
// 30 against 45 m of rise across a Local x-edge). The later domain therefore rises along the seam
// as the neighbour did: `followed` is the rate the neighbour's cell across the seam rose at over its
// window, row by row, and `weight` the share of it a cell takes, all of it at the seam and none a
// block's width in, where the cell's own block mean takes over. A cell near a corner between two
// finished neighbours takes both in proportion to their weights.
void followFinished(const Domain& domain, int span, std::vector<float>& followed, std::vector<float>& weight) {
    const int size = domain.size, low = Halo - 1, high = Halo + TileCells;
    const float window = float(domain.endTime - domain.startTime);
    followed.assign(domain.cells(), 0.0f);
    weight.assign(domain.cells(), 0.0f);
    if (window <= 0) return;
    std::vector<float> share(domain.cells(), 0.0f), open(domain.cells(), 1.0f);
    const auto take = [&](size_t i, size_t across, int depth) {
        if (domain.hold[i] != Free || domain.hold[across] != Settled || depth < 0 || depth >= span) return;
        const float w = 1.0f - float(smoothstep(0.0, double(span), depth + 0.5));
        followed[i] += w * (domain.uplifted[across] / window - domain.residual[across]);
        share[i] += w;
        open[i] *= 1.0f - w;
    };
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            take(i, domain.index(low, z), x - Halo);
            take(i, domain.index(high, z), high - 1 - x);
            take(i, domain.index(x, low), z - Halo);
            take(i, domain.index(x, high), high - 1 - z);
        }
    for (size_t i = 0; i < followed.size(); ++i) {
        if (share[i] <= 0) continue;
        followed[i] /= share[i];
        weight[i] = 1.0f - open[i];
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
        // blocks of parent cells, on the parent's surface against the erosion of the finer level.
        // High ground far above its outlet needs a fast rise and gets steep, dissected relief; ground
        // near its base level needs almost none and stays gentle. The parent's own rate is the start.
        scratch.blurA.resize(domain.cells());
        scratch.blurB.resize(domain.cells());
        for (size_t i = 0; i < domain.cells(); ++i) scratch.blurA[i] = domain.trend[i] - domain.ground.rock[i] - domain.sedimentAt(i);
        const bool ground = domain.key.level == Level::Ground;
        if (ground) blur(scratch.blurA, scratch.blurB, domain.size, TrendRadius);
        else overBlocks(scratch.blurA, domain.hold, domain.size, Refinement * HeldParents);
        // The rate found may hold ground up but never pull it down faster than the parent does: a
        // block that stands above its parent's surface is brought down by its own water and slopes.
        if (!ground && scratch.parentLift.size() != domain.cells()) scratch.parentLift = domain.tectonic;
        for (size_t i = 0; i < domain.cells(); ++i) {
            domain.tectonic[i] += TrendLearn * scratch.blurA[i] / dt;
            domain.rebound[i] = TrendGain * scratch.blurA[i] / dt;
            if (ground) continue;
            const float floor = std::min(0.0f, scratch.parentLift[i]);
            domain.tectonic[i] = std::max(floor, domain.tectonic[i]);
            domain.rebound[i] = std::max(floor - domain.tectonic[i], domain.rebound[i]);
        }
        if (!ground) followFinished(domain, Refinement * HeldParents, scratch.blurA, scratch.blurB);
    }
    const bool follows = !region && domain.key.level != Level::Ground;
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] == Settled) continue;
            const bool pinned = domain.hold[i] == Rim && domain.rimSlope[i] == 0;
            // A ground tile's rock does not sink to make room: what is laid on it stands above the
            // parent's surface until the water takes it on.
            float found = pinned ? 0.0f : domain.tectonic[i] + domain.rebound[i] + domain.residual[i];
            if (follows && !pinned) found = float(mix(found, scratch.blurA[i] + domain.residual[i], scratch.blurB[i]));
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
