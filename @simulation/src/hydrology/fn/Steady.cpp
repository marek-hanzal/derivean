#include "hydrology/fn/Water.hpp"

namespace derivean::sim {

// Evaporation and filling are each solved exactly and applied one after the other. Lowering open
// water by what the air takes is the exact answer to evaporation alone, and settling the inflow of
// the span is the exact answer to filling alone, so the step is stable at any length and a lake
// that overflows ends exactly at its sill. A lake held by evaporation alone ends slightly high.
Lakes fillAndSpill(WaterGrid& grid, const BasinTree& tree, double years, double climateYears, WaterFlows& total) {
    const double area = tree.area;
    WaterFlows flows;
    std::vector<Tally> parcels(tree.basins.size());
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        if (grid.kindOf(int32_t(cell)) != CellKind::Open) continue;
        if (grid.depth[cell] > OpenWater) {
            const double rained = grid.rain[cell] * climateYears;
            const double lost = std::min(grid.depth[cell] + rained, grid.evaporation[cell] * climateYears);
            grid.depth[cell] += rained - lost;
            flows.rain.water += rained * area;
            flows.evaporation.water += lost * area;
        } else {
            Tally& parcel = parcels[size_t(tree.leafOf[cell])];
            const double shed = grid.runoff[cell] * climateYears * area;
            parcel.water += shed;
            parcel.salt += shed * grid.sourceSalinity;
            flows.rain.water += shed;
            flows.rain.salt += shed * grid.sourceSalinity;
        }
    }
    for (const WaterSource& source : grid.sources) {
        Tally& parcel = parcels[size_t(tree.leafOf[size_t(source.cell)])];
        const double entered = source.flow * years;
        parcel.water += entered;
        parcel.salt += entered * grid.sourceSalinity;
        flows.inflow.water += entered;
        flows.inflow.salt += entered * grid.sourceSalinity;
    }
    const Lakes lakes = settle(grid, tree, std::move(parcels), flows);
    total += flows;
    return lakes;
}

void routeDischarge(WaterGrid& grid, const BasinTree& tree, const Lakes& lakes) {
    const size_t cells = grid.cells(), count = tree.basins.size();
    const double area = tree.area;
    std::vector<double> flow(cells, 0.0), rate(count, 0.0), thirst(count, 0.0);
    for (size_t cell = 0; cell < cells; ++cell) {
        if (grid.kindOf(int32_t(cell)) != CellKind::Open) continue;
        if (grid.depth[cell] > OpenWater) {
            // Open water passes its rain on and charges what it evaporates to the lake it lies in.
            flow[cell] = grid.rain[cell] * area;
            const int32_t basin = tree.basinOf[cell];
            if (basin) thirst[size_t(lakes.lakeOf[size_t(basin)])] += grid.evaporation[cell] * area;
        } else {
            flow[cell] = grid.runoff[cell] * area;
        }
    }
    for (const WaterSource& source : grid.sources) flow[size_t(source.cell)] += source.flow;
    // Upstream cells first: every cell hands what passes through it to the neighbour it falls to.
    for (size_t step = tree.order.size(); step-- > 0;) {
        const int32_t cell = tree.order[step], to = tree.receiver[size_t(cell)];
        if (grid.kindOf(cell) != CellKind::Open) continue;
        if (to >= 0 && grid.kindOf(to) == CellKind::Open) flow[size_t(to)] += flow[size_t(cell)];
        else if (to < 0) rate[size_t(lakes.lakeOf[size_t(tree.basinOf[size_t(cell)])])] += flow[size_t(cell)];
    }
    // A full lake hands on what it receives less what it evaporates: across its saddle and down
    // the far side into the next pit.
    for (const int32_t top : lakes.ranked) {
        const Basin& basin = tree.basins[size_t(top)];
        const double passed = rate[size_t(top)] - thirst[size_t(top)];
        if (passed <= 0 || !lakes.full[size_t(top)]) continue;
        if (grid.kindOf(basin.saddle) == CellKind::Open) flow[size_t(basin.saddle)] += passed;
        for (int32_t cell = basin.entry; cell >= 0 && grid.kindOf(cell) == CellKind::Open; cell = tree.receiver[size_t(cell)])
            flow[size_t(cell)] += passed;
        if (basin.over) rate[size_t(lakes.lakeOf[size_t(basin.over)])] += passed;
    }
    for (size_t cell = 0; cell < cells; ++cell) grid.discharge[cell] = float(flow[cell]);
}

} // namespace derivean::sim
