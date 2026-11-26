#pragma once

#include "landscape/fn/Build.hpp"

namespace derivean::sim {

// Where the ground of one tile went over its replay, as metres of mean ground over the tile's own
// cells. Uplift, erosion and deposition are the three movements a tile books both in its ledger
// and cell by cell; `residual` is what the ground did beyond them: solution, creep and slope
// failure across the tile's edge, and the blending of shared edges.
struct TileBudget {
    TileKey key;
    double change{};      // mean ground at the end minus at the start of the replay
    double uplifted{}, eroded{}, deposited{};   // TileLedger over the tile's area
    double residual{};    // change - (uplifted - eroded + deposited)
    double turnover{};    // |uplifted| + eroded + deposited: the scale the residual is read against
    // TileLedger::dissolved: solute lost to weathering, which lowers the ground beyond the three
    // booked movements, together with the solute of rock cut by rivers, which is already part of
    // `eroded`. The first alone would explain the residual; the ledger does not keep them apart.
    double dissolved{};
    double ledgerGap{};   // the same balance from Tile::uplifted, eroded and deposited, minus the ledger's: zero when the two records agree
    // Where the residual comes from: the cells that gained and the cells that lost ground beyond
    // their totals, the share of the gain under standing water at the end, and the largest single cell.
    double gain{}, loss{};
    double gainUnderWater{};   // 0..1
    double worstCell{};        // m, signed
};

// Rebuilds the tile's starting ground with buildDomain, so `lookup` must return its dependencies.
TileBudget budgetOf(const Setting& setting, const Tile& tile, const TileLookup& lookup);

} // namespace derivean::sim
