#include "hydrology/fn/Strait.hpp"

#include "hydrology/fn/Water.hpp"

#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

double strengthOf(const Tally& stock) { return stock.water > 0 ? stock.salt / stock.water : 0.0; }

void mark(Exchange& exchange, const Ocean& ocean, int32_t basin) {
    exchange.touched[size_t(ocean.lakes.lakeOf[size_t(basin)])] = 1;
    exchange.touched[size_t(exchange.lakeAtStart[size_t(basin)])] = 1;
}

} // namespace

Exchange openExchange(const Ocean& ocean) {
    const BasinTree& tree = ocean.basins;
    Exchange exchange;
    exchange.lake.assign(tree.basins.size(), Tally{});
    exchange.lakeAtStart = ocean.lakes.lakeOf;
    exchange.touched.assign(tree.basins.size(), 0);
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell) {
        if (ocean.grid.depth[cell] <= 0) continue;
        Tally& lake = exchange.lake[size_t(ocean.lakes.lakeOf[size_t(tree.basinOf[cell])])];
        lake.water += ocean.grid.depth[cell] * tree.area;
        lake.salt += ocean.grid.salt[cell] * tree.area;
    }
    return exchange;
}

Tally book(Ocean& ocean, Exchange& exchange, int32_t donor, int32_t receiver, int32_t leaf, double volume) {
    std::vector<double>& own = ocean.lakes.own;
    assert(volume >= 0 && volume <= own[size_t(donor)]);
    Tally& from = exchange.lake[size_t(ocean.lakes.lakeOf[size_t(donor)])];
    const Tally carried{volume, volume * strengthOf(from)};
    own[size_t(donor)] -= volume;
    [[maybe_unused]] const double lost = pour(ocean.basins, own, leaf, volume);
    assert(lost == 0 && own[size_t(donor)] >= 0);
    from.water -= carried.water;
    from.salt -= carried.salt;
    exchange.lake[size_t(receiver)] += carried;
    mark(exchange, ocean, donor);
    mark(exchange, ocean, receiver);
    exchange.transfers.push_back({donor, receiver, carried});
    return carried;
}

void relake(Ocean& ocean, Exchange& exchange) {
    const std::vector<int32_t> was = ocean.lakes.lakeOf;
    refreshLakes(ocean.basins, ocean.lakes);
    std::vector<Tally> lake(exchange.lake.size());
    for (size_t id = 1; id < lake.size(); ++id) {
        const double own = ocean.lakes.own[id];
        if (own <= 0) continue;
        Tally& into = lake[size_t(ocean.lakes.lakeOf[id])];
        into.water += own;
        into.salt += own * strengthOf(exchange.lake[size_t(was[id])]);
    }
    exchange.lake.swap(lake);
    // A lake made of one that took part took part too: a sibling that merged with a lake that took
    // part shares its strength from then on, and keeps it when they part again.
    const std::vector<uint8_t> took = exchange.touched;
    for (size_t id = 1; id < lake.size(); ++id)
        if (took[size_t(was[id])]) exchange.touched[size_t(ocean.lakes.lakeOf[id])] = 1;
}

void closeExchange(Ocean& ocean, Exchange& exchange) {
    if (exchange.transfers.empty()) return;
    relake(ocean, exchange);
    WaterGrid& grid = ocean.grid;
    const BasinTree& tree = ocean.basins;
    const double area = tree.area;
    const auto rewritten = [&](size_t cell) {
        const size_t basin = size_t(tree.basinOf[cell]);
        return basin > 0 && (exchange.touched[size_t(exchange.lakeAtStart[basin])] || exchange.touched[size_t(ocean.lakes.lakeOf[basin])]);
    };
    const auto depthOf = [&](size_t cell) { return std::max(0.0, ocean.lakes.level[size_t(ocean.lakes.lakeOf[size_t(tree.basinOf[cell])])] - double(grid.ground[cell])); };

    // Each body that took part is closed over the cells of the largest basin that took part above
    // it: the sets are disjoint, and every transfer leaves one and enters one.
    const size_t count = tree.basins.size();
    std::vector<int32_t> group(count, -1);
    for (size_t id = count; id-- > 1;) {
        const int32_t parent = tree.basins[id].parent;
        group[id] = parent > 0 && group[size_t(parent)] >= 0 ? group[size_t(parent)] : exchange.touched[id] ? int32_t(id) : -1;
    }
    std::vector<Tally> before(count), after(count), booked(count), turnover(count);
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        const int32_t at = tree.basinOf[cell] > 0 ? group[size_t(tree.basinOf[cell])] : -1;
        if (at >= 0) before[size_t(at)] += {grid.depth[cell] * area, grid.salt[cell] * area};
    }
    for (const Transfer& transfer : exchange.transfers) {
        const size_t from = size_t(group[size_t(transfer.from)]), to = size_t(group[size_t(transfer.to)]);
        booked[from].water -= transfer.carried.water, booked[from].salt -= transfer.carried.salt;
        booked[to] += transfer.carried;
        turnover[from] += transfer.carried;
        turnover[to] += transfer.carried;
    }

    // Salt the water now covers dissolves into it; then every cell of a lake that took part stands
    // at its lake's level and strength. A cell that stays dry keeps its crust.
    for (size_t cell = 0; cell < grid.cells(); ++cell)
        if (rewritten(cell) && grid.depth[cell] <= 0 && grid.salt[cell] > 0 && depthOf(cell) > 0)
            exchange.lake[size_t(ocean.lakes.lakeOf[size_t(tree.basinOf[cell])])].salt += grid.salt[cell] * area;
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        if (!rewritten(cell)) continue;
        const double depth = depthOf(cell);
        if (depth <= 0 && grid.depth[cell] <= 0) continue;
        grid.depth[cell] = depth;
        grid.salt[cell] = depth * strengthOf(exchange.lake[size_t(ocean.lakes.lakeOf[size_t(tree.basinOf[cell])])]);
        assert(grid.salt[cell] >= 0);
    }

    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        const int32_t at = tree.basinOf[cell] > 0 ? group[size_t(tree.basinOf[cell])] : -1;
        if (at >= 0) after[size_t(at)] += {grid.depth[cell] * area, grid.salt[cell] * area};
    }
    for (size_t id = 1; id < count; ++id) {
        if (group[id] != int32_t(id)) continue;
        // Against what the body held and what passed through it in this integration.
        const double water = std::abs(after[id].water - before[id].water - booked[id].water) /
                             std::max(1.0, std::max(before[id].water, after[id].water) + turnover[id].water);
        const double salt = std::abs(after[id].salt - before[id].salt - booked[id].salt) /
                            std::max(1e-30, std::max(before[id].salt, after[id].salt) + turnover[id].salt);
        ocean.unbooked.water = std::max(ocean.unbooked.water, water);
        ocean.unbooked.salt = std::max(ocean.unbooked.salt, salt);
    }
    exchange = Exchange{};
}

} // namespace derivean::sim
