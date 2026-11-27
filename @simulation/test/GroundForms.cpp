#include "GroundFixture.hpp"

#include "core/fn/Hash.hpp"
#include "ground/constant/Ground.hpp"
#include "ground/fn/Frost.hpp"
#include "ground/fn/Hydrothermal.hpp"
#include "ground/fn/Rockfall.hpp"
#include "ground/fn/Stages.hpp"
#include "ground/fn/Stones.hpp"

#include <numeric>

using namespace derivean::sim;

namespace {

// A wall of rock over a talus at its angle of repose and a level floor: every block must end below
// where it started, and the large ones farther out than the small.
void cliffShedsSortedBlocks() {
    FallGrid grid;
    grid.cell = 2.0;
    grid.width = 120, grid.height = 60;
    grid.origin = {0, 0};
    const size_t cells = size_t(grid.width) * size_t(grid.height);
    grid.ground.resize(cells), grid.roughness.resize(cells), grid.drag.assign(cells, 0.0f);
    for (int z = 0; z < grid.height; ++z)
        for (int x = 0; x < grid.width; ++x) {
            const double along = (x + 0.5) * grid.cell;
            const double wall = along < 20 ? 140 : along < 40 ? 140 - 3.0 * (along - 20) : along < 140 ? 80 - 0.7 * (along - 40) : 10 - 0.01 * (along - 140);
            grid.ground[size_t(z) * size_t(grid.width) + size_t(x)] = float(wall);
            grid.roughness[size_t(z) * size_t(grid.width) + size_t(x)] = along < 40 ? 0.05f : along < 140 ? 0.35f : 0.12f;
        }
    std::vector<FallSource> sources;
    for (int z = 8; z < 52; z += 2) sources.push_back({{24.0, (z + 0.5) * grid.cell}, 60.0f, {0.6f, 0.8f}, Material::Granite, uint64_t(z)});
    const std::vector<FallenBlock> blocks = shedBlocks(42, grid, sources, 400.0);
    expectGround(blocks.size() > 300, "a breaking wall shed too few blocks", double(blocks.size()));
    std::vector<std::pair<float, float>> runs;   // size, distance out
    for (const FallenBlock& block : blocks) {
        expectGround(grid.heightAt(block.rest) <= grid.heightAt(block.source) + 0.5f, "a block came to rest above its source", grid.heightAt(block.rest));
        runs.push_back({block.size, float(block.rest.x - block.source.x)});
    }
    std::sort(runs.begin(), runs.end());
    const size_t third = runs.size() / 3;
    double small = 0, large = 0;
    for (size_t i = 0; i < third; ++i) small += runs[i].second, large += runs[runs.size() - 1 - i].second;
    expectGround(large > 1.3 * small, "large blocks did not run out farther than small ones", large / std::max(1.0, small));
    // Run-outs are a distribution: blocks neither all stop at the foot nor all leave.
    std::sort(runs.begin(), runs.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    expectGround(runs[runs.size() * 9 / 10].second - runs[runs.size() / 10].second > 20, "blocks all stopped at one distance", runs[runs.size() / 2].second);
}

// A spring on a slope: the mineral it lays must dam pools behind rims that stand in steps down the slope.
void springBuildsTerraces() {
    SpringGround around;
    around.cell = 0.25;
    around.side = 193;
    around.origin = {0, 0};
    around.ground.resize(size_t(around.side) * size_t(around.side));
    for (int z = 0; z < around.side; ++z)
        for (int x = 0; x < around.side; ++x)
            around.ground[size_t(z) * size_t(around.side) + size_t(x)] = float(100.0 - 0.10 * x * around.cell + 0.02 * unit(hash(7, x, z)));
    Spring spring;
    spring.position = {12.0, 24.0};
    spring.discharge = 0.006f, spring.temperature = 72, spring.carbonate = 0.5f, spring.silica = 0.05f;
    growSpringDeposit(spring, around);
    double volume = 0;
    int pools = 0, rims = 0;
    double inPool = 0, onRim = 0;
    std::vector<float> levels;
    for (int z = 1; z < around.side - 1; ++z)
        for (int x = 1; x < around.side - 1; ++x) {
            const size_t i = size_t(z) * size_t(around.side) + size_t(x);
            volume += around.crust[i] * around.cell * around.cell;
            if (around.pool[i] > 0.03f) {
                ++pools, inPool += around.crust[i];
                levels.push_back(around.ground[i] + around.crust[i] + around.pool[i]);
            } else if (around.pool[i + 1] > 0.03f || around.pool[i - 1] > 0.03f || around.pool[i + size_t(around.side)] > 0.03f || around.pool[i - size_t(around.side)] > 0.03f) {
                ++rims, onRim += around.crust[i];
            }
        }
    const double supplied = 0.02 * (0.5 / 2200.0 + 0.05 / 1900.0) * 0.006 * YearSeconds * 600.0;
    expectGround(volume > 0.05 * supplied && volume <= 1.001 * supplied, "a spring laid more mineral than its water carried, or next to none", volume / supplied);
    expectGround(pools > 200 && rims > 60, "a spring on a slope dammed no pools", double(pools));
    expectGround(onRim / rims > 1.3 * inPool / pools, "the rims of spring pools are not thicker than their floors", (onRim / rims) / (inPool / pools));
    // Terraces: the pools stand at several distinct levels down the slope.
    std::sort(levels.begin(), levels.end());
    int steps = 1;
    for (size_t i = 1; i < levels.size(); ++i) steps += levels[i] - levels[i - 1] > 0.04f;
    expectGround(steps >= 3, "spring pools do not step down the slope", double(steps));
}

// The same on a ground tile: the deposit lands on the tile's cells and the spring's water stands in its pools.
void springDepositHoldsWater() {
    const auto plane = [](double x) { return 100.0 - 0.10 * x; };
    GroundSpec spec;
    spec.ground = [&](int x, int) { return float(plane(-Halo * 0.25 + (x + 0.5) * 0.25) - 0.2); };
    spec.fines = 0.2f;
    Domain domain = groundDomain(spec);
    SiteView sites;
    sites.cell = 4.0, sites.firstX = sites.firstZ = -8, sites.side = 40;
    sites.ground.resize(size_t(sites.side) * size_t(sites.side));
    for (int z = 0; z < sites.side; ++z)
        for (int x = 0; x < sites.side; ++x) sites.ground[sites.index(x, z)] = float(plane((sites.firstX + x + 0.5) * sites.cell));
    Spring spring;
    spring.position = {20.0, 32.0};
    spring.discharge = 0.006f, spring.temperature = 88, spring.carbonate = 0.5f, spring.silica = 0.05f, spring.id = 7;
    GroundDetail detail;
    const size_t cells = size_t(TileCells) * TileCells;
    detail.crust.assign(cells, 0.0f);
    detail.crustKind.assign(cells, uint8_t(Crust::None));
    std::vector<float> altered;
    laySpringDeposits(domain, {spring}, sites, detail, altered);
    double volume = 0;
    for (float crust : detail.crust) volume += crust * 0.0625;
    expectGround(volume > 5, "a spring left no deposit on its ground tile", volume);
    expectGround(altered[domain.index(Halo + 80, Halo + 128)] > 0.5f, "the ground at a scalding vent is not altered", altered[domain.index(Halo + 80, Halo + 128)]);
    FlowScratch scratch;
    Tile tile;
    standWater(domain, scratch, {}, detail, tile);
    int pooled = 0;
    for (size_t i = 0; i < cells; ++i) pooled += tile.water[i] > tile.now.rock[i] + tile.now.sediment[Fines][i] + detail.crust[i] + 0.03f;
    expectGround(pooled > 30, "no water stands in the pools a spring dammed", pooled);
}

// Level ground that freezes, with stones and fines mixed: the frost must separate them without changing how much of either there is.
void frozenGroundSorts() {
    GroundSpec spec;
    spec.ground = [](int, int) { return 50.0f; };
    spec.warmth = -4.0f;
    spec.frost = 0.2f;
    Domain domain = groundDomain(spec);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.sediment[Coarse][i] = 0.12f + 0.02f * float(unit(hash(3, x, z)) - 0.5);
            domain.ground.sediment[Fines][i] = 0.25f;
        }
    const auto measure = [&](double& coarse, double& fines, double& spread) {
        coarse = fines = 0;
        double sum = 0, squares = 0;
        for (int z = Halo; z < Halo + TileCells; ++z)
            for (int x = Halo; x < Halo + TileCells; ++x) {
                const size_t i = domain.index(x, z);
                const double share = domain.ground.sediment[Coarse][i] / (domain.ground.sediment[Coarse][i] + domain.ground.sediment[Fines][i]);
                coarse += domain.ground.sediment[Coarse][i], fines += domain.ground.sediment[Fines][i];
                sum += share, squares += share * share;
            }
        const double n = double(TileCells) * TileCells;
        spread = std::sqrt(std::max(0.0, squares / n - (sum / n) * (sum / n)));
    };
    double coarseBefore, finesBefore, spreadBefore, coarseAfter, finesAfter, spreadAfter;
    measure(coarseBefore, finesBefore, spreadBefore);
    FlowScratch scratch;
    floodLevels(domain, scratch);
    std::vector<uint8_t> sorted{1};
    const Ground before = domain.ground;
    expectGround(!workFrozenGround(domain, scratch, sorted, FrostMovement::PassThrough),
                 "disabled frost movement reported that it moved ground");
    expectGround(sorted == std::vector<uint8_t>(domain.cells(), 0), "disabled frost movement marked sorted cells");
    expectGround(domain.ground.rock == before.rock && domain.ground.datum == before.datum,
                 "disabled frost movement changed the rock");
    for (int layer = 0; layer < SedimentClasses; ++layer)
        expectGround(domain.ground.sediment[layer] == before.sediment[layer],
                     "disabled frost movement changed the mantle");
    workFrozenGround(domain, scratch, sorted);
    measure(coarseAfter, finesAfter, spreadAfter);
    expectGround(std::abs(coarseAfter - coarseBefore) < 1e-3 * coarseBefore && std::abs(finesAfter - finesBefore) < 1e-3 * finesBefore, "frost sorting made or lost mantle", coarseAfter - coarseBefore);
    expectGround(spreadAfter > 5 * spreadBefore, "freezing ground did not sort its stones from its fines", spreadAfter / spreadBefore);
    // Borders around centres, not a salt-and-pepper mix: a cell agrees with its neighbour far more often than chance.
    int same = 0, pairs = 0;
    for (int z = Halo + 8; z < Halo + TileCells - 8; ++z)
        for (int x = Halo + 8; x < Halo + TileCells - 8; ++x) {
            const auto stony = [&](int cx, int cz) { return domain.ground.sediment[Coarse][domain.index(cx, cz)] > 0.12f; };
            same += stony(x, z) == stony(x + 1, z), ++pairs;
        }
    expectGround(same > 0.8 * pairs, "sorted ground has no pattern larger than a cell", double(same) / pairs);
}

// A steep channel with a bed of boulders: the bed must break into steps with water standing below them, at uneven spacing.
void steepChannelFormsSteps() {
    GroundSpec spec;
    spec.rock = Material::Granite;
    spec.ground = [](int x, int z) { return float(200.0 - 0.14 * 0.25 * x + 0.9 * std::min(1.0, std::abs(z - DomainCells / 2) / 14.0)); };
    spec.coarse = 0.7f, spec.sand = 0.1f, spec.fines = 0.03f;
    Domain domain = groundDomain(spec);
    domain.inflow[domain.index(2, DomainCells / 2)] = 6.0e6f;
    FlowScratch scratch;
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    widenChannels(domain);
    StoneBed bed{domain, 42, {}, std::vector<uint8_t>(domain.cells(), 0), {}, {}};
    SiteView none;
    none.cell = 4.0, none.side = 1, none.rock.assign(1, uint8_t(Material::Granite)), none.known.assign(1, 0), none.flowTo.assign(1, 255), none.discharge.assign(1, 0.0f);
    const TileLookup lookup = [](TileKey) -> const Tile* { return nullptr; };
    const double before = [&] { double sum = 0; for (size_t i = 0; i < domain.cells(); ++i) sum += domain.ground.rock[i] + domain.sedimentAt(i); return sum; }();
    packFabric(bed, scratch, none, lookup);
    expectGround(bed.stones.size() > 100, "a boulder bed was laid without stones", double(bed.stones.size()));
    organiseBeds(bed, scratch);
    double after = 0, stones = 0;
    for (size_t i = 0; i < domain.cells(); ++i) after += domain.ground.rock[i] + domain.sedimentAt(i);
    for (const Clast& stone : bed.stones) stones += stone.volume();
    expectGround(std::abs((after - before) * 0.0625 + stones) < 0.01 * stones + 0.05, "stones and their bed do not add up to the bed before", (after - before) * 0.0625 + stones);
    expectGround(bed.stepGaps.size() >= 4, "a steep boulder bed formed no steps", double(bed.stepGaps.size()));
    const double mean = std::accumulate(bed.stepGaps.begin(), bed.stepGaps.end(), 0.0) / bed.stepGaps.size();
    double squares = 0;
    for (float gap : bed.stepGaps) squares += (gap - mean) * (gap - mean);
    expectGround(std::sqrt(squares / bed.stepGaps.size()) > 0.15 * mean, "steps stand at even spacing", std::sqrt(squares / bed.stepGaps.size()) / mean);

    GroundDetail detail;
    detail.crust.assign(size_t(TileCells) * TileCells, 0.0f);
    Tile tile;
    standWater(domain, scratch, bed.stones, detail, tile);
    int pooled = 0;
    for (size_t i = 0; i < tile.water.size(); ++i) {
        const float ground = tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i];
        pooled += tile.water[i] > ground + 0.2f;
    }
    expectGround(pooled > 40, "no water stands in the pools of a stepped bed", double(pooled));
}

} // namespace

void testGroundForms() {
    cliffShedsSortedBlocks();
    springBuildsTerraces();
    springDepositHoldsWater();
    frozenGroundSorts();
    steepChannelFormsSteps();
}
