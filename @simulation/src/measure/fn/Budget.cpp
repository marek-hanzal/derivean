#include "measure/fn/Budget.hpp"

#include "icesheet/type/Sheet.hpp"

#include <cmath>

namespace derivean::sim {

TileBudget budgetOf(const Setting& setting, const Tile& tile, const TileLookup& lookup) {
    TileBudget budget;
    budget.key = tile.key;
    const Domain start = buildDomain(setting, tile.key, lookup);
    // The parent's last-sheet deposits are set aside while the tile settles (icesheet/fn/Parent.cpp).
    const bool held = start.sheet && start.sheet->held[0].size() == start.cells();
    const double cells = double(TileCells) * TileCells;
    const double area = cells * start.cell * start.cell;
    double before = 0, after = 0, booked = 0, wetGain = 0;
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = start.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            double was = double(start.ground.rock[from]) + start.sedimentAt(from);
            if (held) was += double(start.sheet->held[Coarse][from]) + start.sheet->held[Sand][from] + start.sheet->held[Fines][from];
            const double now = double(tile.now.rock[to]) + tile.now.sediment[0][to] + tile.now.sediment[1][to] + tile.now.sediment[2][to];
            const double moved = double(tile.uplifted[to]) - tile.eroded[to] + tile.deposited[to];
            before += was, after += now, booked += moved;
            const double beyond = now - was - moved;
            (beyond > 0 ? budget.gain : budget.loss) += beyond / cells;
            if (beyond > 0 && tile.water[to] > now) wetGain += beyond / cells;
            if (std::abs(beyond) > std::abs(budget.worstCell)) budget.worstCell = beyond;
        }
    }
    budget.change = (after - before) / cells;
    budget.uplifted = tile.ledger.uplifted / area;
    budget.eroded = tile.ledger.eroded / area;
    budget.deposited = tile.ledger.deposited / area;
    budget.dissolved = tile.ledger.dissolved / area;
    const double ledger = budget.uplifted - budget.eroded + budget.deposited;
    budget.residual = budget.change - ledger;
    budget.turnover = std::abs(budget.uplifted) + budget.eroded + budget.deposited;
    budget.ledgerGap = booked / cells - ledger;
    if (budget.gain > 0) budget.gainUnderWater = wetGain / budget.gain;
    return budget;
}

} // namespace derivean::sim
