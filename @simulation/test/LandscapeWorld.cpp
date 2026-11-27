#include "LandscapeFixture.hpp"

#include "measure/fn/Budget.hpp"
#include "measure/fn/Dependencies.hpp"
#include "measure/fn/Determinism.hpp"

using namespace derivean::sim;

namespace {

// Ledger and per-cell totals are two records of the same work and may only differ by rounding.
// At region level uplift, erosion and deposition also explain where the ground went, up to what
// left in solution; the finer levels move too much ground by slope processes that keep no account
// to be held to that yet.
void checkBudget(Level level, double residualLimit) {
    World& world = testWorld();
    const TileKey key = tileAt(level, testFocus());
    const TileBudget budget = budgetOf(world.atlas.setting(), *world.atlas.require(key), requireDependencies(world, key).lookup());
    const double scale = std::max(1e-3, budget.turnover);
    expectLandscape(std::abs(budget.ledgerGap) < 1e-3 * scale, "a tile's ledger disagrees with its per-cell totals", budget.ledgerGap);
    if (residualLimit <= 0) return;
    expectLandscape(budget.residual < residualLimit * scale, "a region tile gained ground that nothing booked", budget.residual / scale);
    expectLandscape(budget.residual > -budget.dissolved - residualLimit * scale, "a region tile lost more ground than erosion and solution booked", budget.residual / scale);
}

// The fresh world compares the focus chain, which is first-pass at every level. A pass-3 tile next
// to it waits for all eight neighbours of its level: replayed on this thread alone, without the
// pool, against the world's finished neighbours, it must come out the same to the bit.
void checkLastPass(Level level) {
    World& world = testWorld();
    const TileKey home = tileAt(level, testFocus());
    TileKey key = home;
    for (int dz = 0; dz <= 1; ++dz)
        for (int dx = 0; dx <= 1; ++dx)
            if (passOf({level, home.x + dx, home.z + dz}) == 3) key = {level, home.x + dx, home.z + dz};
    expectLandscape(passOf(key) == 3, "no pass-3 tile next to the focus", double(int(level)));
    const std::shared_ptr<const Tile> built = world.atlas.require(key);
    Setting alone = world.atlas.setting();
    alone.workers = nullptr;
    const TileDifference difference = differenceOf(*built, *replayTile(alone, key, requireDependencies(world, key).lookup()));
    expectLandscape(difference.cells == 0, "a pass-3 tile depends on the number of threads", double(difference.cells));
}

} // namespace

void testLandscapeWorld() {
    World& world = testWorld();
    // The fresh one-thread world compares the planet and every Region, Local and Site tile of the
    // chain, but cuts no voxels: ground tiles and their ground water are compared against one
    // thread in testGroundTile, testGroundwaterTile and testGroundwaterRegion, and the later passes
    // in checkLastPass, over this world's finished inputs, instead of building the whole 3 x 3
    // neighbourhood again on one thread.
    const DeterminismReport report = checkDeterminism(world, testFocus(), Level::Site, Level::Site, 1, false, false);
    expectLandscape(report.planetSame, "the planet depends on the number of threads");
    expectLandscape(report.tiles.size() >= 5, "too few tiles were compared", double(report.tiles.size()));
    for (const DeterminismReport::Entry& entry : report.tiles)
        expectLandscape(entry.difference.cells == 0, "a tile depends on threads or on the order it was asked for", double(entry.difference.cells));
    expectLandscape(report.replayed.cells == 0, "replaying a tile twice gives two results", double(report.replayed.cells));
    expectLandscape(report.blockCells > 0, "no block of voxels was cut across the ground-tile edge", double(report.blockCells));

    checkLastPass(Level::Region);
    checkLastPass(Level::Local);

    checkBudget(Level::Region, 0.05);
    checkBudget(Level::Local, 0);
    checkBudget(Level::Site, 0);
}
