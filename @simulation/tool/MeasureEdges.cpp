#include "Measure.hpp"

#include "measure/fn/Budget.hpp"
#include "measure/fn/Dependencies.hpp"
#include "measure/fn/Levels.hpp"
#include "measure/fn/Seams.hpp"

#include <cstdio>

using namespace derivean::sim;

void printSeams(const Mosaic& mosaic) {
    const SeamReport seams = seamsOf(mosaic, riverThreshold(mosaic.level));
    std::printf("seams (1 = invisible): step %.2f (%.3f m at edges, %.3f inside)  bend %.2f (%.3f, %.3f)  lakes %.2f (%.3f %% near, %.3f %% far)\n",
                seams.stepRatio, seams.stepAcross, seams.stepInside, seams.bendRatio, seams.bendAcross, seams.bendInside, seams.lakeRatio,
                100 * seams.lakeNear, 100 * seams.lakeFar);
    std::printf("  rivers crossing edges: %d  discharge kept %.2f (median %.3f across, %.3f inside)  lost >30 %%: %.1f %% across, %.1f %% inside\n",
                seams.crossings, seams.keptRatio, seams.keptAcross, seams.keptInside, 100 * seams.lostAcross, 100 * seams.lostInside);
    std::printf("    into a tile solved earlier: %d crossings, %.0f %% lost | into a tile solved later: %d crossings, %.0f %% lost\n", seams.intoEarlier,
                100 * seams.lostIntoEarlier, seams.intoLater, 100 * seams.lostIntoLater);
}

void printLevels(World& world, const Mosaic& child) {
    const Mosaic parent = gatherParents(world, child);
    const LevelReport levels = levelsOf(parent, child, riverThreshold(parent.level));
    std::printf("levels: child mean minus parent over %d parent cells: mean %.2f  rms %.2f  p05 %.2f  p50 %.2f  p95 %.2f  extremes %.1f .. %.1f m"
                "  (parent cells step %.2f m)\n",
                levels.parentCells, levels.drift.mean, levels.rms, levels.drift.p05, levels.drift.p50, levels.drift.p95, levels.drift.low,
                levels.drift.high, levels.parentStep);
    std::printf("  parent rivers %d: found within 1 parent cell %.0f %%, within 2 %.0f %%  standing water agrees in %.1f %% of parent cells\n",
                levels.rivers, 100 * levels.foundNear, 100 * levels.foundWide, 100 * levels.waterAgree);
}

void printBudget(World& world, const Mosaic& mosaic) {
    std::printf("budget, m of mean ground per tile: change = uplifted - eroded + deposited + residual; dissolved is what the ledger says left in solution\n");
    double worst = 0;
    for (int dz = 0; dz < mosaic.tiles; ++dz) {
        for (int dx = 0; dx < mosaic.tiles; ++dx) {
            const TileKey key{mosaic.level, mosaic.first.x + dx, mosaic.first.z + dz};
            const TileBudget budget = budgetOf(world.atlas.setting(), *world.atlas.find(key), requireDependencies(world, key).lookup());
            const double share = budget.turnover > 0 ? budget.residual / budget.turnover : 0.0;
            worst = std::max(worst, std::abs(share));
            std::printf("  tile %d %d pass %d: change %+.3f = %+.3f - %.3f + %.3f %+.3f (%.1f %% of turnover; dissolved %.3f) | gains %+.3f, %.0f %% under water; losses %.3f; worst cell %+.1f m | ledger vs cells %+.4f\n",
                        key.x, key.z, passOf(key), budget.change, budget.uplifted, budget.eroded, budget.deposited, budget.residual, 100 * share,
                        budget.dissolved, budget.gain, 100 * budget.gainUnderWater, budget.loss, budget.worstCell, budget.ledgerGap);
        }
    }
    std::printf("  worst residual %.1f %% of turnover\n", 100 * worst);
}
