#include "LandscapeFixture.hpp"

#include "landscape/fn/Process.hpp"
#include "landscape/fn/Strength.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Block.hpp"
#include "measure/fn/Plane.hpp"
#include "measure/fn/ReliefStats.hpp"

#include <array>
#include <cstdio>
#include <utility>

using namespace derivean::sim;

namespace {

// m a face of this rock holds over a horizontal run; `sound` is its cohesion against the usual.
float holds(Material rock, double run, double sound = 1) {
    const float scale = std::pow(float(sound), 0.75f);
    return stableDrop(strengthOf(rock), std::log2(float(run)), scale, std::log2(scale));
}

double degreesOver(Material rock, double run, double sound = 1) { return std::atan(holds(rock, run, sound) / run) * DegreesPerRadian; }

// m of rock and loose cover in the whole domain, as a column sum.
double groundIn(const Domain& domain) {
    double sum = 0;
    for (size_t i = 0; i < domain.cells(); ++i) sum += double(domain.ground.rock[i]) + domain.ground.sediment[0][i] + domain.ground.sediment[1][i] + domain.ground.sediment[2][i];
    return sum;
}

// The greatest height a bare cell stands above a neighbour, over what a fresh face of its rock holds across that run.
double worstFace(const Domain& domain, double sound) {
    double worst = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) {
            const size_t i = domain.index(x, z);
            if (domain.hold[i] != Free || domain.sedimentAt(i) > 1) continue;
            for (int k = 0; k < 8; ++k) {
                const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                const double drop = domain.ground.rock[i] - domain.ground.rock[other] - domain.sedimentAt(other);
                worst = std::max(worst, drop / holds(domain.unit[i].traits.dominant, NeighborReach[k] * domain.cell, sound));
            }
        }
    return worst;
}

// m of ground the water took off the tile's own cells, as a mean over them.
double erodedFrom(const Domain& domain) { return domain.ledger.eroded / (domain.cell * domain.cell * TileCells * TileCells); }

// How loose cover stands over a grid: the most a cell's cover rises above the angle of repose of
// its mix over its lowest neighbour (a needle of debris shows here), and the thickest cover.
struct LooseStand {
    double unsupported{}, thickest{};
};

// `at(x, z)` gives a cell's rock and its three sediment classes; cells on the outer ring only serve
// as neighbours.
template<typename At>
LooseStand looseStand(int size, double cell, At&& at) {
    LooseStand stand;
    for (int z = 1; z < size - 1; ++z)
        for (int x = 1; x < size - 1; ++x) {
            const auto [rock, sediment] = at(x, z);
            double loose = 0, repose = 0;
            for (int c = 0; c < SedimentClasses; ++c) loose += sediment[c], repose += ReposeTan[c] * sediment[c];
            if (loose <= 0) continue;
            stand.thickest = std::max(stand.thickest, loose);
            repose /= loose;
            for (int k = 0; k < 8; ++k) {
                const auto [otherRock, otherSediment] = at(x + NeighborX[k], z + NeighborZ[k]);
                const double drop = rock + loose - otherRock - otherSediment[0] - otherSediment[1] - otherSediment[2];
                stand.unsupported = std::max(stand.unsupported, std::min(loose, drop - repose * NeighborReach[k] * cell));
            }
        }
    return stand;
}

LooseStand looseStand(const Domain& domain) {
    return looseStand(TileCells, domain.cell, [&](int x, int z) {
        const size_t i = domain.index(Halo - 1 + x, Halo - 1 + z);
        return std::pair{double(domain.ground.rock[i]), std::array<double, SedimentClasses>{domain.ground.sediment[0][i], domain.ground.sediment[1][i], domain.ground.sediment[2][i]}};
    });
}

LooseStand looseStand(const Tile& tile, double cell) {
    return looseStand(TileCells, cell, [&](int x, int z) {
        const size_t i = size_t(z) * TileCells + size_t(x);
        return std::pair{double(tile.now.rock[i]), std::array<double, SedimentClasses>{tile.now.sediment[0][i], tile.now.sediment[1][i], tile.now.sediment[2][i]}};
    });
}

BlockSpec canyon(Material rock, double sound, int steps) {
    BlockSpec spec;
    spec.level = Level::Site, spec.rock = rock, spec.sound = sound;
    spec.height = 200, spec.floorHalf = 12, spec.steps = steps;
    return spec;
}

} // namespace

// What a wall may do, on ground where the answer is known beforehand.
void testRelief() {
    // The strength the slopes are judged by: sound granite stands as a wall of the order of a hundred
    // metres and massive granite of a few hundred; over a kilometre both lean back towards their
    // friction angle; shale holds neither. No rock holds a steeper mean slope over a longer run.
    expectLandscape(holds(Material::Granite, 1) > 50 && holds(Material::Granite, 1) < 150, "the wall granite holds is off", holds(Material::Granite, 1));
    expectLandscape(holds(Material::Granite, 1, 4) > 180 && holds(Material::Granite, 1, 4) < 350, "the wall massive granite holds is off", holds(Material::Granite, 1, 4));
    expectLandscape(degreesOver(Material::Granite, 1000) > 34 && degreesOver(Material::Granite, 1000) < 46, "a kilometre of granite slope stands at the wrong angle", degreesOver(Material::Granite, 1000));
    expectLandscape(holds(Material::Shale, 1) < 15 && degreesOver(Material::Shale, 1000) < 25, "shale holds a wall or a steep mountainside", degreesOver(Material::Shale, 1000));
    for (int id = 0; id < MaterialCount; ++id) {
        if (definitionOf(Material(id)).massCohesion <= 0) continue;
        for (double run = 1; run < 1e5; run *= 2) {
            expectLandscape(holds(Material(id), 2 * run) >= holds(Material(id), run), "a longer run holds a smaller drop", id);
            expectLandscape(holds(Material(id), 2 * run) / (2 * run) <= holds(Material(id), run) / run * 1.0001, "a longer run holds a steeper slope", id);
        }
        // One cell of the Local level cannot stand at more than the angle its height allows.
        expectLandscape(degreesOver(Material(id), 64, 4) < 82, "a 64 m cell of the strongest rock stands near vertical", degreesOver(Material(id), 64, 4));
    }

    // Ground that has gathered no channel is lowered by the water it has. A plane without rain keeps
    // its rock; gentle granite under rain loses metres to tens of metres in a million years, and its
    // mantle never lends the rock under it its own erodibility.
    PlaneSpec gentle;
    gentle.level = Level::Region, gentle.rock = Material::Granite, gentle.slope = 0.002, gentle.steps = 5, gentle.rain = 0;
    const double dry = erodedFrom(replayPlane(gentle));
    expectLandscape(dry < 0.01, "a plane without rain was lowered", dry);
    gentle.rain = 0.9;
    const double perMyr = erodedFrom(replayPlane(gentle)) / (gentle.steps * RegionStep / Myr);
    expectLandscape(perMyr > 1 && perMyr < 40, "gentle granite under rain is lowered at the wrong pace", perMyr);

    // Massive granite cut by a river: a wall stands, and no bare cell stands higher above a neighbour
    // than a fresh face of its rock holds.
    Domain strong = replayBlock(canyon(Material::Granite, 4, 12));
    expectLandscape(tallestFace(strong, std::tan(60 / DegreesPerRadian)) > 80, "massive granite does not hold a wall", tallestFace(strong, std::tan(60 / DegreesPerRadian)));
    {
        FlowScratch scratch;
        shapeSlopes(strong, scratch);
        expectLandscape(worstFace(strong, 4) < 1.02, "a face stands higher than its rock holds", worstFace(strong, 4));
    }
    // The same canyon twice is the same canyon.
    const Domain again = replayBlock(canyon(Material::Granite, 4, 12));
    expectLandscape(again.ground.rock == replayBlock(canyon(Material::Granite, 4, 12)).ground.rock, "a block replayed twice gives two results");

    // Left to its slopes alone, a wall higher than its rock holds comes down to what it holds, and
    // what fell lies under it at the angle debris rests at. Nothing is lost on the way.
    BlockSpec loose = canyon(Material::Granite, 1, 10);
    loose.river = false, loose.height = 180, loose.floorHalf = 30;
    Domain fallen = blockDomain(loose);
    FlowScratch scratch;
    const double before = groundIn(fallen);
    for (int step = 0; step < loose.steps; ++step) stepBlock(fallen, scratch, loose);
    const double area = fallen.cell * fallen.cell, moved = fallen.ledger.failed / area;
    expectLandscape(moved > 1e5, "a wall twice as high as granite holds did not fail", moved);
    expectLandscape(std::abs(before - groundIn(fallen) - scratch.left / area) < 2e-3 * moved, "slope processes lost or made ground", before - groundIn(fallen) - scratch.left / area);
    floodLevels(fallen, scratch);
    const Mosaic apron = mosaicOf(fallen);
    const CoverShares cover = coverShares(apron, slopeField(apron), steepestFall(apron));
    expectLandscape(cover.talusAngle.count > 500, "a failed wall left no talus", double(cover.talusAngle.count));
    // The body of an apron rests at repose. Its steepest twentieth does not yet: what the last
    // round of settleDebris leaves on cells the pass had already visited stands until the next step.
    expectLandscape(cover.talusAngle.p50 > 28 && cover.talusAngle.p50 < 39, "talus does not lie at its angle of repose", cover.talusAngle.p50);

    // Shale holds no wall.
    BlockSpec weak = loose;
    weak.rock = Material::Shale;
    Domain soft = replayBlock(weak);
    expectLandscape(tallestFace(soft, std::tan(60 / DegreesPerRadian)) < 15, "shale holds a wall", tallestFace(soft, std::tan(60 / DegreesPerRadian)));

}

// Strong beds over weak ones give a wall in steps: many short faces, none of them the whole height.
void testReliefBeds() {
    BlockSpec bedded = canyon(Material::Sandstone, 2, 20);
    bedded.weak = Material::Shale, bedded.bedThickness = 24;
    const Domain steps = replayBlock(bedded);
    const Mosaic stepped = mosaicOf(steps);
    const CliffStats cliffs = cliffInventory(stepped, steepestFall(stepped));
    expectLandscape(cliffs.faces > 40 && cliffs.height.p50 > 5 && cliffs.height.p50 < 50, "bedded rock did not break into steps", cliffs.height.p50);
    expectLandscape(tallestFace(steps, std::tan(60 / DegreesPerRadian)) < 0.6 * bedded.height, "bedded rock stands as one wall", tallestFace(steps, std::tan(60 / DegreesPerRadian)));
    // What fell from the steps lies under them at repose; it never stands as a column of debris.
    const LooseStand debris = looseStand(steps);
    expectLandscape(debris.unsupported < 2 * steps.cell, "debris stands steeper than its repose", debris.unsupported);
    expectLandscape(debris.thickest < bedded.height, "debris piles higher than the wall it fell from", debris.thickest);
}

// Loose cover on finished tiles of an ordinary world rests at its angle of repose: no cell holds
// more than a few metres of cover above what repose lets it lean on its lowest neighbour, and none
// carries hundreds of metres of it. What leaves a tile as sediment it eroded or took in.
void testReliefCover() {
    World& world = testWorld();
    for (const Level level : {Level::Local, Level::Site}) {
        const double cell = specOf(level).cell;
        const Tile& tile = *world.atlas.require(tileAt(level, testFocus()));
        const LooseStand stand = looseStand(tile, cell);
        expectLandscape(stand.unsupported < std::max(4.0, 0.25 * cell), "loose cover stands steeper than its repose on a finished tile", stand.unsupported);
        expectLandscape(stand.thickest < 100, "a finished tile carries a column of loose cover", stand.thickest);
        const TileLedger& ledger = tile.ledger;
        expectLandscape(ledger.sedimentOut <= ledger.eroded + ledger.sedimentIn + 1, "a tile sends out more sediment than it eroded and took in", ledger.sedimentOut / (ledger.eroded + ledger.sedimentIn + 1));
    }
}

// A Site tile of the present world under an Alpine range the glaciers went over: what the last ice
// left rests at repose like other cover, and no cell of rock stands as a needle above all the
// ground around it. Both are what an ice-sheet's Leave pass and one-cell fins used to leave. The
// place is a trough wall on seed 42 (`terra view` from 5606144, 10107264 towards it).
void testReliefGlaciatedSite() {
    World& world = testWorldPresent();
    const Tile& tile = *world.atlas.require(tileAt(Level::Site, {5605504, 10105024}));
    const double cell = specOf(Level::Site).cell;
    float iceAge = 0;
    for (const float ice : tile.iceAge) iceAge = std::max(iceAge, ice);
    expectLandscape(iceAge > 50, "the glaciated Site tile saw no glacier", iceAge);
    const LooseStand stand = looseStand(tile, cell);
    expectLandscape(stand.unsupported < 4, "loose cover stands steeper than its repose on a glaciated Site tile", stand.unsupported);
    expectLandscape(stand.thickest < 100, "a glaciated Site tile carries a column of loose cover", stand.thickest);
    const auto top = [&](int x, int z) {
        const size_t i = size_t(z) * TileCells + size_t(x);
        return double(tile.now.rock[i]) + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i];
    };
    double needle = 0;
    for (int z = 1; z < TileCells - 1; ++z)
        for (int x = 1; x < TileCells - 1; ++x) {
            double high = -1e30;
            for (int k = 0; k < 8; ++k) high = std::max(high, top(x + NeighborX[k], z + NeighborZ[k]));
            needle = std::max(needle, top(x, z) - high);
        }
    expectLandscape(needle < 6 * cell, "a cell of rock stands as a needle above all its neighbours", needle);
}

// A Local tile on a coast where rivers that cross finished neighbours reach the sea: what leaves it
// as sediment it eroded or took in. The load a finished neighbour's record carries into its own sea
// is that neighbour's, and the tile's plume must not take it again.
void testReliefLedger() {
    World& world = testWorldPresent();
    const TileLedger& ledger = world.atlas.require({Level::Local, 705, 261})->ledger;
    std::printf("relief: coast tile 705 261 eroded %.3g m3, took in %.3g, sent out %.3g\n", ledger.eroded, ledger.sedimentIn, ledger.sedimentOut);
    expectLandscape(ledger.eroded > 0 && ledger.sedimentOut > 0, "the coast tile neither eroded nor sent out sediment", ledger.eroded);
    expectLandscape(ledger.sedimentOut <= ledger.eroded + ledger.sedimentIn + 1, "a coast tile sends out more sediment than it eroded and took in", ledger.sedimentOut / (ledger.eroded + ledger.sedimentIn + 1));
}
