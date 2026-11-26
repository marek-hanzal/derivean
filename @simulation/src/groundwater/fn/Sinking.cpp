#include "groundwater/fn/Sinking.hpp"

#include "groundwater/fn/Conductivity.hpp"
#include "groundwater/fn/Recharge.hpp"
#include "groundwater/fn/Swallow.hpp"
#include "landscape/fn/Process.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr float OpenDepth = 10.0f;      // m of weathered, opened joints: the rock mass is not known at this level
// Water that gathers on soluble rock has taken up most of what it can carry where it fell. A hollow
// that takes it in dissolves at most this many times as fast as bare ground beside it: dolines
// deepen until their slopes bring in as much as the floor loses.
constexpr float DolineGain = 4.0f;

// A Region cell is wider than most soluble bodies' springs lie apart; one rising per body would
// drain a platform across its watersheds. The Region keeps its water; the ground level takes what
// its parent left.
bool sinksAt(Level level) { return level == Level::Local || level == Level::Site; }

} // namespace

void prepareSinks(Domain& domain) {
    if (!sinksAt(domain.key.level)) return;
    const size_t cells = domain.cells();
    const int size = domain.size;
    SinkWork& work = domain.sinking;
    bool any = false;
    for (size_t i = 0; i < cells && !any; ++i) any = fissuredBySolution(definitionOf(domain.unit[i].traits.dominant));
    if (!any && !work.active()) return;
    const bool fresh = !work.active();
    if (fresh) work.received.assign(cells, -1.0f), work.sunk.assign(cells, 0.0f), work.rising.assign(cells, -1);
    // The risings of the step before, until the water sunk then has been brought out.
    std::vector<int32_t>& before = work.outlet;
    before = work.rising;
    work.rate.assign(cells, 0.0f), work.soak.assign(cells, 0.0f), work.rising.assign(cells, -1);
    // What the rock alone would take in, kept in `soak` until the bodies are known.
    std::vector<float>& base = work.soak;
    for (size_t i = 0; i < cells; ++i)
        if (fissuredBySolution(definitionOf(domain.unit[i].traits.dominant)))
            base[i] = swallowRate({domain.unit[i].traits.dominant, domain.sedimentAt(i), std::clamp(domain.weakness[i], 0.0f, 1.0f), OpenDepth, 0.0f});

    // Soluble bodies, each the land cells it joins by any of the eight neighbours, labelled in
    // index order. A body rises at its lowest free cell that meets other rock, the sea or the rim:
    // water that comes out there runs off over ground that does not take it in again. A body with
    // no such cell in the domain takes nothing in. A lake on a body is part of it; its floor leaks.
    const auto wet = [&](size_t i) { return domain.surface[i] < float(domain.seaLevel); };
    std::vector<int32_t>& body = work.body;
    std::vector<int32_t>& members = work.members;
    body.assign(cells, -1);
    for (size_t seed = 0, bodies = 0; seed < cells; ++seed) {
        if (base[seed] <= 0 || body[seed] >= 0 || wet(seed)) continue;
        members.assign(1, int32_t(seed));
        body[seed] = int32_t(bodies);
        int32_t rising = -1;
        for (size_t at = 0; at < members.size(); ++at) {
            const int32_t cell = members[at];
            const int x = cell % size, z = cell / size;
            bool edge = domain.hold[size_t(cell)] == Rim;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) {
                    edge = true;
                    continue;
                }
                const size_t other = domain.index(nx, nz);
                if (base[other] <= 0 || wet(other)) {
                    edge = true;
                    continue;
                }
                if (body[other] < 0) body[other] = int32_t(bodies), members.push_back(int32_t(other));
            }
            if (edge && domain.hold[size_t(cell)] != Settled && (rising < 0 || domain.surface[size_t(cell)] < domain.surface[size_t(rising)])) rising = cell;
        }
        ++bodies;
        if (rising < 0) continue;
        const float floor = domain.surface[size_t(rising)];
        for (const int32_t cell : members) {
            work.rising[size_t(cell)] = rising;
            if (domain.hold[size_t(cell)] != Free) continue;
            work.rate[size_t(cell)] = base[size_t(cell)] * float(smoothstep(SinkRisingZone, 2.0 * SinkRisingZone, domain.surface[size_t(cell)] - floor));
        }
    }

    // Rain soaks into soluble rock where it takes water in at all, by the share the ground water
    // model reads (groundwater/fn/Columns.cpp).
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            work.soak[i] = 0;
            if (work.rate[i] <= 0) continue;
            const auto at = [&](int ox, int oz) { return domain.surface[domain.index(std::clamp(x + ox, 0, size - 1), std::clamp(z + oz, 0, size - 1))]; };
            const float slope = float(std::hypot(at(1, 0) - at(-1, 0), at(0, 1) - at(0, -1)) / (2 * domain.cell));
            const Soak soak{domain.rain[i], domain.warmthNow[i], slope,
                            {domain.ground.sediment[Coarse][i], domain.ground.sediment[Sand][i], domain.ground.sediment[Fines][i]}, domain.ice[i], domain.unit[i].traits.dominant};
            work.soak[i] = infiltrationShare(soak);
        }

    // The water that went down in the step before comes out where its body rises now: had it come
    // out where the body rose then, it could run into the body again. Water of a cell that is no
    // longer soluble comes out where it was bound.
    work.risen.assign(cells, 0.0f);
    for (size_t i = 0; !fresh && i < cells; ++i) {
        const int32_t to = work.rising[i] >= 0 ? work.rising[i] : before[i];
        if (work.sunk[i] > 0 && to >= 0) work.risen[size_t(to)] += work.sunk[i];
    }

    // A hollow drains down where its floor takes in all that reached it in the step before: what
    // it took while it was open, what its lake passed to the outlet while it held one, or else
    // what passed through it. The flood of the step before reached every cell of a lake from the
    // one it spilled from, so in its order the cell a lake spills over is known before the lake's.
    std::vector<int32_t>& outlet = work.outlet;
    outlet.assign(cells, -1);
    for (const int32_t cell : domain.order) outlet[size_t(cell)] = domain.spill[size_t(cell)] >= 0 ? outlet[size_t(domain.spill[size_t(cell)])] : cell;
    const float area = float(domain.cell * domain.cell);
    for (size_t i = 0; i < cells; ++i) {
        float inflow = work.received[i] >= 0 ? work.received[i] : domain.discharge[i];
        if (domain.spill[i] >= 0 && outlet[i] >= 0) inflow = domain.discharge[size_t(outlet[i])];
        // Within the zone above the rising the water in the rock stands too near the ground for a
        // hollow to empty: there it holds a lake, which may run out over the rising.
        const bool above = work.rate[i] > 0 && domain.surface[i] >= domain.surface[size_t(work.rising[i])] + SinkRisingZone;
        work.received[i] = !fresh && above && work.rate[i] * area >= inflow ? inflow : -1.0f;
    }
    std::fill(work.sunk.begin(), work.sunk.end(), 0.0f);
    work.total = 0;
}

bool drainsDown(const Domain& domain, size_t cell) {
    const SinkWork& work = domain.sinking;
    if (!work.active() || work.received[cell] < 0 || domain.hold[cell] != Free || domain.surface[cell] <= float(domain.seaLevel)) return false;
    const int size = domain.size, x = int(cell % size_t(size)), z = int(cell / size_t(size));
    for (int k = 0; k < 8; ++k) {
        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
        if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
        if (domain.surface[domain.index(nx, nz)] < domain.surface[cell]) return false;
    }
    return true;
}

float soakAt(Domain& domain, size_t cell, float own) {
    SinkWork& work = domain.sinking;
    if (!work.active() || work.soak[cell] <= 0 || own <= 0) return 0;
    const float soaked = own * work.soak[cell];
    work.sunk[cell] += soaked;
    work.total += soaked;
    return soaked;
}

float sinkAt(Domain& domain, size_t cell, float water, bool standing, bool pit) {
    SinkWork& work = domain.sinking;
    if (!work.active() || work.rate[cell] <= 0 || water <= 0) return 0;
    const float width = float(domain.cell);
    const float lost = pit ? water : standing ? std::min(water, work.rate[cell] * width * width) : swallowed(work.rate[cell], water, width);
    if (pit) work.received[cell] = water;
    work.sunk[cell] += lost;
    work.total += lost;
    return lost;
}

void dissolveSinks(Domain& domain) {
    SinkWork& work = domain.sinking;
    if (!work.active()) return;
    const float dt = float(domain.step), area = float(domain.cell * domain.cell);
    const int size = domain.size;
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (work.sunk[i] <= 0 || domain.hold[i] != Free) continue;
        // A cell's own runoff already dissolves it (Hillslope.cpp); this is the water gathered from elsewhere.
        const float gathered = std::min(std::max(0.0f, work.sunk[i] - domain.runoff[i] * area), (DolineGain - 1) * domain.runoff[i] * area);
        float lowered = sinkSolution(domain.unit[i].traits.solubility, gathered, float(domain.cell)) * dt;
        // Karst does not lower the sill of a lake the parent holds below the parent's water.
        if (!domain.lakeFloor.empty()) lowered = std::min(lowered, std::max(0.0f, domain.ground.rock[i] + domain.sedimentAt(i) - domain.lakeFloor[i]));
        domain.ground.rock[i] -= lowered;
        if (domain.owned(int(i % size_t(size)), int(i / size_t(size)))) domain.ledger.dissolved += double(lowered) * area;
    }
}

} // namespace derivean::sim
