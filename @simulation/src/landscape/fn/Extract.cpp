#include "landscape/fn/Build.hpp"

#include "aeolian/fn/Wind.hpp"
#include "coast/fn/Coast.hpp"

#include <algorithm>
#include <limits>

namespace derivean::sim {
namespace {

// Copies a square of the domain that reaches `margin` cells beyond the tile's own cells.
void copyGround(const Domain& domain, Ground& into, int margin) {
    const int side = TileCells + 2 * margin;
    into.resize(size_t(side) * size_t(side));
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            const size_t from = domain.index(x + Halo - margin, z + Halo - margin), to = size_t(z) * size_t(side) + size_t(x);
            into.rock[to] = domain.ground.rock[from];
            into.datum[to] = domain.ground.datum[from];
            // A layer a process has used up can stand a rounding step below nothing; a tile hands none on.
            for (int c = 0; c < SedimentClasses; ++c) into.sediment[c][to] = std::max(0.0f, domain.ground.sediment[c][from]);
        }
    }
}

// Near an edge shared with a finished neighbour, the tile's own cells give way to what that
// neighbour's replay found for the same cells. Both replays worked against the same finished
// ground, so they differ only in detail; fading from one to the other leaves no line on the map.
void blendSeams(const Domain& domain, Ground& core) {
    Ground Tile::*wide = &Tile::nowWide;
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const int sideX = x < Apron ? -1 : x >= TileCells - Apron ? 1 : 0, sideZ = z < Apron ? -1 : z >= TileCells - Apron ? 1 : 0;
            const Tile* alongX = sideX ? domain.finished[4 + sideX] : nullptr;
            const Tile* alongZ = sideZ ? domain.finished[4 + 3 * sideZ] : nullptr;
            if (!alongX && !alongZ) continue;
            const float wx = alongX ? seamWeight(sideX < 0 ? x : TileCells - 1 - x) : 1.0f;
            const float wz = alongZ ? seamWeight(sideZ < 0 ? z : TileCells - 1 - z) : 1.0f;
            const Tile* diagonal = alongX && alongZ ? domain.finished[4 + sideX + 3 * sideZ] : nullptr;
            const size_t to = size_t(z) * TileCells + size_t(x);
            // This cell in the frame of a neighbour's wide square.
            const auto there = [&](int dx, int dz) { return size_t(z - dz * TileCells + Apron) * WideCells + size_t(x - dx * TileCells + Apron); };
            const auto mixIn = [&](std::vector<float>& own, const std::vector<float> Ground::*member) {
                const float fromX = alongX ? ((alongX->*wide).*member)[there(sideX, 0)] : own[to];
                const float fromZ = alongZ ? ((alongZ->*wide).*member)[there(0, sideZ)] : own[to];
                const float fromBoth = diagonal ? ((diagonal->*wide).*member)[there(sideX, sideZ)] : 0.5f * (fromX + fromZ);
                own[to] = own[to] * wx * wz + fromX * (1 - wx) * wz + fromZ * wx * (1 - wz) + fromBoth * (1 - wx) * (1 - wz);
            };
            mixIn(core.rock, &Ground::rock);
            mixIn(core.datum, &Ground::datum);
            for (int c = 0; c < SedimentClasses; ++c) {
                const float fromX = alongX ? (alongX->*wide).sediment[c][there(sideX, 0)] : core.sediment[c][to];
                const float fromZ = alongZ ? (alongZ->*wide).sediment[c][there(0, sideZ)] : core.sediment[c][to];
                const float fromBoth = diagonal ? (diagonal->*wide).sediment[c][there(sideX, sideZ)] : 0.5f * (fromX + fromZ);
                core.sediment[c][to] = core.sediment[c][to] * wx * wz + fromX * (1 - wx) * wz + fromZ * wx * (1 - wz) + fromBoth * (1 - wx) * (1 - wz);
            }
        }
    }
}

// Water decides first: the blend may neither dam a course that either side drains through nor dig a
// hollow into one. Down the steepest way of the domain's own ground from each cell, and along every
// river a finished neighbour hands on, a blended cell stays no higher than the cells draining into
// it and no lower than the cell it drains to. Where the blend would break that, it is taken back
// toward the tile's own ground until the course runs again.
void keepCourses(const Domain& domain, Ground& core) {
    const auto groundAt = [&](size_t i) {
        float s = domain.ground.rock[i];
        for (int c = 0; c < SedimentClasses; ++c) s += std::max(0.0f, domain.ground.sediment[c][i]);
        return s;
    };
    const auto coreOf = [&](size_t i) {
        const int x = int(i % size_t(domain.size)) - Halo, z = int(i / size_t(domain.size)) - Halo;
        return x < 0 || z < 0 || x >= TileCells || z >= TileCells ? -1 : int64_t(z) * TileCells + x;
    };
    const auto receiver = [&](size_t i) -> int64_t {
        const int x = int(i % size_t(domain.size)), z = int(i / size_t(domain.size));
        if (domain.handsOnward(i)) return int64_t(domain.index(x + NeighborX[domain.settledWay[i]], z + NeighborZ[domain.settledWay[i]]));
        const float here = groundAt(i);
        float steepest = 0;
        int64_t to = -1;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= domain.size || nz >= domain.size) continue;
            const size_t n = domain.index(nx, nz);
            const float slope = (here - groundAt(n)) / NeighborReach[k];
            if (slope > steepest) steepest = slope, to = int64_t(n);
        }
        return to;
    };
    const size_t cells = size_t(TileCells) * TileCells;
    std::vector<float> was(cells), now(cells);
    std::vector<size_t> moved;
    for (size_t to = 0; to < cells; ++to) {
        was[to] = groundAt(domain.index(int(to % TileCells) + Halo, int(to / TileCells) + Halo));
        now[to] = core.rock[to];
        for (int c = 0; c < SedimentClasses; ++c) now[to] += core.sediment[c][to];
        if (now[to] != was[to]) moved.push_back(to);
    }
    const auto heightOf = [&](size_t i) { const int64_t c = coreOf(i); return c < 0 ? groundAt(i) : now[size_t(c)]; };
    std::sort(moved.begin(), moved.end(), [&](size_t a, size_t b) { return was[a] < was[b] || (was[a] == was[b] && a < b); });
    // Downstream first, a lowered cell is held at the cell it drains to.
    for (const size_t to : moved) {
        if (now[to] >= was[to]) continue;
        const int64_t r = receiver(domain.index(int(to % TileCells) + Halo, int(to / TileCells) + Halo));
        if (r >= 0) now[to] = std::max(now[to], std::min(was[to], heightOf(size_t(r))));
    }
    // Upstream first, a raised cell is held at the lowest cell that drains into it.
    for (size_t at = moved.size(); at-- > 0;) {
        const size_t to = moved[at];
        if (now[to] <= was[to]) continue;
        const int x = int(to % TileCells) + Halo, z = int(to / TileCells) + Halo;
        const size_t i = domain.index(x, z);
        float cap = std::numeric_limits<float>::max();
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= domain.size || nz >= domain.size) continue;
            const size_t n = domain.index(nx, nz);
            if (receiver(n) == int64_t(i)) cap = std::min(cap, heightOf(n));
        }
        now[to] = std::max(was[to], std::min(now[to], cap));
    }
    for (const size_t to : moved) {
        float blended = core.rock[to];
        for (int c = 0; c < SedimentClasses; ++c) blended += core.sediment[c][to];
        if (blended == now[to]) continue;
        const float keep = (now[to] - was[to]) / (blended - was[to]);
        const size_t own = domain.index(int(to % TileCells) + Halo, int(to / TileCells) + Halo);
        core.rock[to] = domain.ground.rock[own] + keep * (core.rock[to] - domain.ground.rock[own]);
        core.datum[to] = domain.ground.datum[own] + keep * (core.datum[to] - domain.ground.datum[own]);
        for (int c = 0; c < SedimentClasses; ++c) {
            const float base = std::max(0.0f, domain.ground.sediment[c][own]);
            core.sediment[c][to] = base + keep * (core.sediment[c][to] - base);
        }
    }
}

} // namespace

void settleSeams(Domain& domain, Tile& tile) {
    Ground core;
    copyGround(domain, core, 0);
    copyGround(domain, tile.nowWide, Apron);
    blendSeams(domain, core);
    keepCourses(domain, core);
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = size_t(z) * TileCells + size_t(x), to = domain.index(x + Halo, z + Halo);
            domain.ground.rock[to] = core.rock[from];
            domain.ground.datum[to] = core.datum[from];
            for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][to] = core.sediment[c][from];
        }
    }
}

void extractTile(const Domain& domain, const FlowScratch& scratch, Tile& tile) {
    const size_t cells = size_t(TileCells) * TileCells;
    tile.key = domain.key;
    tile.time = domain.time;
    tile.seaLevel = domain.seaLevel;
    tile.ledger = domain.ledger;
    tile.aeolian = windRecordOf(domain);
    copyGround(domain, tile.now, 0);
    for (auto* field : {&tile.water, &tile.discharge, &tile.load, &tile.iceAge, &tile.iceSpeed, &tile.uplift, &tile.rebound, &tile.ice, &tile.erosionRate, &tile.depositionRate,
                        &tile.eroded, &tile.deposited, &tile.uplifted, &tile.rain, &tile.warmth, &tile.shed})
        field->resize(cells);
    tile.upIce = domain.upIce;
    tile.shore = recordShore(domain);
    tile.flowTo.assign(cells, 255);
    if (domain.sinking.active()) tile.sunk.assign(cells, 0.0f);
    if (!domain.seeped.empty()) tile.seeped.assign(cells, 0.0f);
    tile.rock.assign(cells, uint8_t(Material::Bedrock));
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            // Standing water shows where water is left after evaporation: the sea, and the parts of a
            // basin that its rivers still reach.
            const bool flooded = domain.level[from] > domain.surface[from] + 1e-3f;
            const bool sea = domain.level[from] <= float(domain.seaLevel) + 0.5f;
            tile.water[to] = flooded && (sea || domain.discharge[from] > 0) ? domain.level[from] : domain.surface[from] - 1.0f;
            tile.discharge[to] = domain.discharge[from];
            if (!tile.sunk.empty()) tile.sunk[to] = domain.sinking.sunk[from];
            if (!tile.seeped.empty()) tile.seeped[to] = domain.seeped[from];
            tile.rock[to] = uint8_t(domain.unit[from].traits.dominant);
            // A replay of zero steps (a world at its very beginning) has moved no sediment yet.
            tile.load[to] = scratch.flux[0].empty() ? 0.0f : scratch.flux[0][from] + scratch.flux[1][from] + scratch.flux[2][from];
            tile.uplift[to] = domain.uplift[from];
            tile.rebound[to] = domain.rebound[from];
            tile.ice[to] = domain.ice[from];
            tile.iceAge[to] = domain.iceAge[from];
            tile.iceSpeed[to] = domain.iceSpeed[from];
            tile.erosionRate[to] = domain.erosionRate[from];
            tile.depositionRate[to] = domain.depositionRate[from];
            tile.eroded[to] = domain.eroded[from];
            tile.deposited[to] = domain.deposited[from];
            tile.uplifted[to] = domain.uplifted[from];
            tile.rain[to] = domain.rain[from];
            tile.warmth[to] = domain.warmthNow[from];
            tile.shed[to] = domain.shed.empty() ? 0.0f : domain.shed[from];
            const float* share = &scratch.share[from * 8];
            int main = -1;
            for (int k = 0; k < 8; ++k)
                if (share[k] > 0 && (main < 0 || share[k] > share[main])) main = k;
            if (main >= 0) tile.flowTo[to] = uint8_t(main);
        }
    }
}

void followNeighbours(Domain& domain, double time) {
    // What crosses the edge of a ground tile are rills and channels, and they grow headward: a tile
    // must see where its finished neighbours' ended from its first step to have the whole settling
    // time to carry them on. So a ground tile takes those cells as they ended, at once.
    const bool atOnce = domain.key.level == Level::Ground;
    const float done = atOnce || domain.endTime <= domain.startTime ? 1.0f : float(saturate((time - domain.startTime) / (domain.endTime - domain.startTime)));
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (domain.hold[i] != Settled && !(domain.hold[i] == Rim && domain.rimSlope[i] == 0)) continue;
        domain.ground.rock[i] = float(mix(domain.settledFrom.rock[i], domain.settledTo.rock[i], done));
        domain.ground.datum[i] = float(mix(domain.settledFrom.datum[i], domain.settledTo.datum[i], done));
        for (int c = 0; c < SedimentClasses; ++c)
            domain.ground.sediment[c][i] = float(mix(domain.settledFrom.sediment[c][i], domain.settledTo.sediment[c][i], done));
    }
}

} // namespace derivean::sim
