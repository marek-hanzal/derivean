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

} // namespace

void testLandscapeWorld() {
    World& world = testWorld();
    const DeterminismReport report = checkDeterminism(world, testFocus(), Level::Site, Level::Site, 1, false);
    expectLandscape(report.planetSame, "the planet depends on the number of threads");
    expectLandscape(report.tiles.size() >= 5, "too few tiles were compared", double(report.tiles.size()));
    for (const DeterminismReport::Entry& entry : report.tiles)
        expectLandscape(entry.difference.cells == 0, "a tile depends on threads or on the order it was asked for", double(entry.difference.cells));
    expectLandscape(report.replayed.cells == 0, "replaying a tile twice gives two results", double(report.replayed.cells));
    expectLandscape(report.blockCells > 0 && report.blockFresh == 0, "a block of voxels differs in a fresh world", double(report.blockFresh));

    checkBudget(Level::Region, 0.05);
    checkBudget(Level::Local, 0);
    checkBudget(Level::Site, 0);
}
