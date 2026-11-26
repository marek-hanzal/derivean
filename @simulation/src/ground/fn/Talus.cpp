#include "ground/fn/Stones.hpp"

#include "core/fn/Hash.hpp"
#include "ground/fn/Partings.hpp"
#include "ground/fn/Rockfall.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

constexpr float FallWindow = 20e3f;      // yr of a face's waste that still lies on top of what it shed before
constexpr float LeastShed = 2e-6f;       // m/yr below which a face is not breaking up
constexpr float ScreeRough = 0.35f;      // m, pieces of a coarse cover
constexpr float SoilRough = 0.12f, SoilDrag = 0.12f;
constexpr float RockRough = 0.05f;
constexpr float WaterDrag = 1.5f;
constexpr int SettleSteps = 10;          // ground cells a block may still shift into the hollow under it

FallGrid gridOf(const SiteView& sites) {
    FallGrid grid;
    grid.origin = {double(sites.firstX) * sites.cell, double(sites.firstZ) * sites.cell};
    grid.cell = sites.cell;
    grid.width = grid.height = sites.side;
    grid.ground = sites.ground;
    grid.roughness.resize(sites.ground.size());
    grid.drag.resize(sites.ground.size());
    for (size_t i = 0; i < sites.ground.size(); ++i) {
        const bool scree = sites.coarse[i] > 0.3f, soil = sites.fine[i] > 0.3f;
        grid.roughness[i] = scree ? ScreeRough : soil ? SoilRough : RockRough;
        grid.drag[i] = sites.wet[i] ? WaterDrag : soil && !scree ? SoilDrag : 0.0f;
    }
    return grid;
}

} // namespace

void dropBlocks(StoneBed& bed, const SiteView& sites, const Setting& setting) {
    std::vector<FallSource> sources;
    const float area = float(sites.cell * sites.cell);
    // A block does not climb: a face lower than every site cell the tile lies on cannot reach it,
    // and neither can one farther off than a block is followed.
    const int own = TileCells / Refinement, first = FallReachCells, last = FallReachCells + own - 1;
    float lowest = 1e30f;
    for (int z = first - 1; z <= last + 1; ++z)
        for (int x = first - 1; x <= last + 1; ++x) lowest = std::min(lowest, sites.ground[sites.index(x, z)]);
    const double farthest = (FallReachCells - 1) * sites.cell;
    for (int z = 0; z < sites.side; ++z)
        for (int x = 0; x < sites.side; ++x) {
            const size_t i = sites.index(x, z);
            if (!sites.known[i] || sites.shed[i] < LeastShed) continue;
            // A block starts anywhere on its cell, no higher than the highest of the cells around it.
            float highest = sites.ground[i];
            for (int k = 0; k < 8; ++k)
                if (sites.inside(x + NeighborX[k], z + NeighborZ[k])) highest = std::max(highest, sites.ground[sites.index(x + NeighborX[k], z + NeighborZ[k])]);
            if (highest < lowest) continue;
            const int awayX = std::max({first - x, x - last, 0}), awayZ = std::max({first - z, z - last, 0});
            if (std::hypot(awayX, awayZ) * sites.cell > farthest + sites.cell) continue;
            const Material rock = Material(sites.rock[i]);
            const Vec2 at = sites.centre(x, z);
            const StrataColumn column = strataAt(*setting.planet, planetPoint(setting.seed, at));
            const StrataUnit unit = unitAt(column, sites.cut[i], foldAt(setting.seed, column, column.material));
            const Bed face = bedAt(setting.seed, column, unit, sites.cut[i], column.material);
            sources.push_back({at, sites.shed[i] * FallWindow * area, blockHabitOf(massOf(setting.seed, column, face, unit.laidDown), traitsOf(rock)), rock,
                               hash(0x534f5552, sites.firstX + x, sites.firstZ + z)});
        }
    if (sources.empty()) return;
    const FallGrid grid = gridOf(sites);
    const Domain& domain = bed.domain;
    std::vector<FallenBlock> blocks = shedBlocks(bed.seed, grid, sources, farthest, bed.domain.workers);
    // The largest first: a small block does not keep a large one from the place both reached.
    std::stable_sort(blocks.begin(), blocks.end(), [](const FallenBlock& a, const FallenBlock& b) { return a.size > b.size; });
    for (const FallenBlock& block : blocks) {
        if (!bed.owns(block.rest)) continue;
        // The site surface brought it here; on the ground it still drops into the hollow next to it.
        size_t cell = bed.cellAt(block.rest);
        for (int step = 0; step < SettleSteps; ++step) {
            const int x = int(cell % size_t(domain.size)), z = int(cell / size_t(domain.size));
            size_t lowest = cell;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (!domain.owned(nx, nz)) continue;
                const size_t other = domain.index(nx, nz);
                if (bed.groundAt(other) < bed.groundAt(lowest) - 0.04f * NeighborReach[k]) lowest = other;
            }
            if (lowest == cell) break;
            cell = lowest;
        }
        const Vec2 rest = domain.center(int(cell % size_t(domain.size)), int(cell / size_t(domain.size)));
        const bool soft = domain.ground.sediment[Sand][cell] + domain.ground.sediment[Fines][cell] > 0.3f;
        StoneOffer offer;
        offer.at = rest;
        offer.size = block.size;
        offer.flatness = block.flatness;
        // Corners break on the way down.
        offer.roundness = std::min(0.3f, 0.04f + 0.002f * block.travel);
        offer.sink = soft ? 0.3f : 0.12f;
        offer.heading = block.heading;
        offer.rock = block.rock;
        offer.origin = ClastOrigin::Fall;
        offer.source = block.source;
        offer.travel = block.travel;
        offer.draw = block.draw;
        layStone(bed, offer);
    }
}

} // namespace derivean::sim
