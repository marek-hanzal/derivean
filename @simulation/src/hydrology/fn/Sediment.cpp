#include "hydrology/fn/Water.hpp"

#include "landscape/fn/Process.hpp"

namespace derivean::sim {
namespace {

float erodibilityOf(const WaterGrid& grid, size_t cell) {
    for (const int layer : {int(Fines), int(Sand), int(Coarse)})
        if (grid.solid.sediment[layer][cell] > 0) return SedimentErodibility[layer];
    return grid.erodibility[cell];
}

} // namespace

double cutColumn(Ground& solid, size_t cell, double wanted) {
    double removed = 0;
    for (const int layer : {int(Fines), int(Sand), int(Coarse)}) {
        float& cover = solid.sediment[layer][cell];
        const float before = cover;
        if (before <= 0) continue;   // a replayed tile may carry a hair of negative cover: nothing to take
        cover = float(std::max(0.0, double(before) - std::max(0.0, wanted - removed)));
        removed += double(before) - double(cover);
    }
    float& rock = solid.rock[cell];
    const float before = rock;
    rock = float(double(before) - std::max(0.0, wanted - removed));
    return removed + double(before) - double(rock);
}

double layColumn(Ground& solid, size_t cell, double wanted, SedimentClass layer) {
    float& cover = solid.sediment[layer][cell];
    const float before = cover;
    cover = float(double(before) + std::max(0.0, wanted));
    return double(cover) - double(before);
}

double streamPower(const WaterGrid& grid, const BasinTree& tree, size_t cell) {
    const int32_t to = tree.receiver[cell];
    if (to < 0 || grid.discharge[cell] <= ChannelHead) return 0;
    const double drop = double(tree.height[cell]) - double(tree.height[size_t(to)]);
    const int32_t step = std::abs(to - int32_t(cell));
    const double reach = grid.cell * (step == 1 || step == grid.width ? 1.0 : 1.41421356);
    return (std::pow(double(grid.discharge[cell]), StreamExponent) - std::pow(double(ChannelHead), StreamExponent)) * drop / reach;
}

// An approximation, not a replay of the landscape model. A bed is taken to be in balance with the
// stream power the untouched surface gave it, so only the excess cuts: more water on the same
// slope, or the same water down a step someone dug. It is explicit in time and limited to part of
// the drop to the next cell. Load is carried down the fall line without loss and dropped where it
// meets standing water, as a lobe that grows outwards from the entry until it stands just under
// the surface; what a lake has no room for goes on with its overflow. Hillslope supply inside the
// grid, sorting by grain size and wash load passing a lake are not modelled.
bool moveSediment(WaterGrid& grid, const BasinTree& tree, const Lakes& lakes, double years, SolidFlows& total) {
    const size_t cells = grid.cells();
    SolidFlows flows;   // of this call; added to the running total once
    const double area = tree.area;
    std::vector<double> load(cells, 0.0);     // m³ arriving on dry cells over the span
    std::vector<double> raised(cells, 0.0);   // m a delta has already laid on a cell in this span
    std::vector<int32_t> seen(cells, -1), front;
    std::vector<uint8_t> done(cells, 0);
    int32_t lobes = 0;
    bool changed = false;

    // Spreads `volume` from `start` over the standing water around it; returns what found room.
    const auto layDelta = [&](int32_t start, double volume) {
        double laid = 0;
        const int32_t lobe = lobes++;
        front.assign(1, start);
        seen[size_t(start)] = lobe;
        for (size_t at = 0; at < front.size() && laid < volume && at < size_t(DeltaReach); ++at) {
            const size_t cell = size_t(front[at]);
            const double room = grid.depth[cell] - DeltaFreeboard - raised[cell];
            if (room > 0) {
                const double height = layColumn(grid.solid, cell, std::min(room, (volume - laid) / area), Sand);
                grid.ground[cell] = groundOf(grid.solid, cell);
                raised[cell] += height;
                laid += height * area;
            }
            for (int k = 0; k < 8; ++k) {
                const int32_t other = grid.neighbour(int32_t(cell), k);
                if (other < 0 || seen[size_t(other)] == lobe || grid.kindOf(other) != CellKind::Open || grid.depth[size_t(other)] <= 0) continue;
                seen[size_t(other)] = lobe;
                front.push_back(other);
            }
        }
        return laid;
    };
    // A load reaching `cell`: a dry cell carries it on when its turn comes, standing water takes
    // what it has room for and hands the rest to the next lake down, if it overflows at all.
    const auto arrive = [&](int32_t cell, double volume) {
        while (volume > 0) {
            if (cell < 0 || grid.kindOf(cell) != CellKind::Open || (grid.depth[size_t(cell)] <= 0 && done[size_t(cell)])) break;
            if (grid.depth[size_t(cell)] <= 0) {
                load[size_t(cell)] += volume;
                return;
            }
            const double laid = layDelta(cell, volume);
            flows.deposited += laid;
            volume -= laid;
            changed = changed || laid != 0;
            const int32_t top = lakes.lakeOf[size_t(tree.basinOf[size_t(cell)])];
            if (!lakes.full[size_t(top)]) break;
            cell = tree.basins[size_t(top)].entry;
        }
        flows.exported += volume;
    };

    for (const WaterSource& source : grid.sources) {
        flows.carriedIn += source.load * years;
        arrive(source.cell, source.load * years);
    }
    // Dry cells from the highest down: each has received all its load before it passes it on.
    for (size_t step = tree.order.size(); step-- > 0;) {
        const int32_t cell = tree.order[step];
        const size_t i = size_t(cell);
        if (grid.kindOf(cell) != CellKind::Open || grid.depth[i] > 0) continue;
        done[i] = 1;
        const int32_t to = tree.receiver[i];
        const double drive = streamPower(grid, tree, i) - grid.balance[i];
        if (drive > 0) {
            const double drop = double(tree.height[i]) - double(tree.height[size_t(to)]);
            const double removed = cutColumn(grid.solid, i, std::min(erodibilityOf(grid, i) * drive * years, CutShare * drop));
            grid.ground[i] = groundOf(grid.solid, i);
            load[i] += removed * area;
            flows.eroded += removed * area;
            changed = changed || removed != 0;
        }
        if (load[i] <= 0) {
            flows.exported += load[i];
            continue;
        }
        if (to >= 0) {
            arrive(to, load[i]);
            continue;
        }
        // A dry pit keeps what fits under its sill; if it overflows without holding water, the rest goes on.
        const Basin& leaf = tree.basins[size_t(tree.basinOf[i])];
        const double laid = layColumn(grid.solid, i, std::min(leaf.spill - double(tree.height[i]), load[i] / area), Sand) * area;
        grid.ground[i] = groundOf(grid.solid, i);
        flows.deposited += laid;
        changed = changed || laid != 0;
        const int32_t top = lakes.lakeOf[size_t(tree.basinOf[i])];
        if (lakes.full[size_t(top)]) arrive(tree.basins[size_t(top)].entry, load[i] - laid);
        else flows.exported += load[i] - laid;
    }
    total.carriedIn += flows.carriedIn, total.eroded += flows.eroded, total.deposited += flows.deposited, total.exported += flows.exported;
    return changed;
}

} // namespace derivean::sim
