#include "hydrology/fn/Strait.hpp"

#include "hydrology/fn/Water.hpp"

#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

double strengthOf(const Tally& stock) { return stock.water > 0 ? stock.salt / stock.water : 0.0; }

Tally negated(const Tally& flow) { return {-flow.water, -flow.salt}; }

void mark(Exchange& exchange, const Ocean& ocean, int32_t basin) {
    exchange.touched[size_t(ocean.lakes.lakeOf[size_t(basin)])] = 1;
    exchange.touched[size_t(exchange.lakeAtStart[size_t(basin)])] = 1;
}

// Books what entered the body `lake` (negative: what left it).
void enter(Exchange& exchange, int32_t lake, const Tally& flow) {
    exchange.account[size_t(lake)] += flow;
    exchange.turnover[size_t(lake)] += {std::abs(flow.water), std::abs(flow.salt)};
}

// The salt of a dry cell is its crust. Once water covers a cell that was dry when the integration
// began, the crust dissolves into the lake over it, before anything else happens to that lake.
void dissolve(Ocean& ocean, Exchange& exchange, int32_t lake, int32_t cell) {
    WaterGrid& grid = ocean.grid;
    if (grid.depth[size_t(cell)] > 0 || grid.salt[size_t(cell)] <= 0) return;
    const Tally crust{0.0, grid.salt[size_t(cell)] * ocean.basins.area};
    grid.salt[size_t(cell)] = 0;
    exchange.lake[size_t(lake)] += crust;
    enter(exchange, lake, crust);
}

// The receiver's surface rose. Its members are wet below its surface; when it held no water of
// its own before, so are the cells under it that lie at its base, which its full children's
// surface only reached. A cell that dries again keeps no crust: its salt left with the water.
void wetten(Ocean& ocean, Exchange& exchange, int32_t basin, bool first) {
    const BasinTree& tree = ocean.basins;
    const Basin& at = tree.basins[size_t(basin)];
    const int32_t lake = ocean.lakes.lakeOf[size_t(basin)];
    const double level = levelOf(tree, basin, ocean.lakes.own[size_t(basin)]);
    const auto heightOf = [&](const Basin& of, int32_t member) { return double(tree.height[size_t(tree.members[size_t(of.first + member)])]); };
    for (int32_t& next = exchange.scanned[size_t(basin)]; next < at.count && heightOf(at, next) < level; ++next)
        dissolve(ocean, exchange, lake, tree.members[size_t(at.first + next)]);
    if (!first || at.child < 0 || !(level > at.base)) return;
    std::vector<int32_t> under{at.child, tree.basins[size_t(at.child)].sibling};
    while (!under.empty()) {
        const Basin& child = tree.basins[size_t(under.back())];
        under.pop_back();
        for (int32_t member = child.count; member-- > 0 && heightOf(child, member) >= at.base;)
            dissolve(ocean, exchange, lake, tree.members[size_t(child.first + member)]);
        if (child.child >= 0 && child.base >= at.base) under.insert(under.end(), {child.child, tree.basins[size_t(child.child)].sibling});
    }
}

} // namespace

Exchange openExchange(const Ocean& ocean) {
    const BasinTree& tree = ocean.basins;
    const size_t count = tree.basins.size();
    Exchange exchange;
    exchange.lake.assign(count, Tally{});
    exchange.account.assign(count, Tally{});
    exchange.turnover.assign(count, Tally{});
    exchange.lakeAtStart = ocean.lakes.lakeOf;
    exchange.scanned.assign(count, 0);
    exchange.touched.assign(count, 0);
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell) {
        if (ocean.grid.depth[cell] <= 0) continue;
        Tally& lake = exchange.lake[size_t(ocean.lakes.lakeOf[size_t(tree.basinOf[cell])])];
        lake.water += ocean.grid.depth[cell] * tree.area;
        lake.salt += ocean.grid.salt[cell] * tree.area;
    }
    exchange.start = exchange.lake;
    return exchange;
}

Tally book(Ocean& ocean, Exchange& exchange, int32_t donor, int32_t receiver, int32_t leaf, double volume) {
    std::vector<double>& own = ocean.lakes.own;
    assert(volume >= 0 && volume <= own[size_t(donor)]);
    // A basin with room is the top of its own lake.
    const int32_t from = ocean.lakes.lakeOf[size_t(donor)], to = ocean.lakes.lakeOf[size_t(receiver)];
    assert(to == receiver);
    const Tally carried{volume, volume * strengthOf(exchange.lake[size_t(from)])};
    const bool first = own[size_t(receiver)] <= 0;
    own[size_t(donor)] -= volume;
    [[maybe_unused]] const double lost = pour(ocean.basins, own, leaf, volume);
    assert(lost == 0 && own[size_t(donor)] >= 0);
    exchange.lake[size_t(from)] += negated(carried);
    exchange.lake[size_t(to)] += carried;
    enter(exchange, from, negated(carried));
    enter(exchange, to, carried);
    mark(exchange, ocean, donor);
    mark(exchange, ocean, receiver);
    ++exchange.segments;
    wetten(ocean, exchange, receiver, first);
    return carried;
}

void relake(Ocean& ocean, Exchange& exchange) {
    const std::vector<int32_t> was = ocean.lakes.lakeOf;
    refreshLakes(ocean.basins, ocean.lakes);
    std::vector<Tally> lake(exchange.lake.size());
    for (size_t id = 1; id < lake.size(); ++id) {
        const double own = ocean.lakes.own[id];
        if (own <= 0) continue;
        const int32_t now = ocean.lakes.lakeOf[id];
        const Tally share{own, own * strengthOf(exchange.lake[size_t(was[id])])};
        lake[size_t(now)] += share;
        // A basin's water that is now in another lake goes from the one's account to the other's.
        if (now == was[id]) continue;
        enter(exchange, was[id], negated(share));
        enter(exchange, now, share);
    }
    exchange.lake.swap(lake);
    // A lake made of one that took part took part too: a sibling that merged with a lake that took
    // part shares its strength from then on, and keeps it when they part again.
    const std::vector<uint8_t> took = exchange.touched;
    for (size_t id = 1; id < lake.size(); ++id)
        if (took[size_t(was[id])]) exchange.touched[size_t(ocean.lakes.lakeOf[id])] = 1;
}

void closeExchange(Ocean& ocean, Exchange& exchange) {
    if (exchange.segments == 0) return;
    relake(ocean, exchange);
    WaterGrid& grid = ocean.grid;
    const BasinTree& tree = ocean.basins;
    const double area = tree.area;
    const auto lakeOf = [&](size_t cell) { return size_t(ocean.lakes.lakeOf[size_t(tree.basinOf[cell])]); };
    const auto rewritten = [&](size_t cell) {
        const size_t basin = size_t(tree.basinOf[cell]);
        return basin > 0 && (exchange.touched[size_t(exchange.lakeAtStart[basin])] || exchange.touched[lakeOf(cell)]);
    };
    const auto depthOf = [&](size_t cell) { return std::max(0.0, ocean.lakes.level[lakeOf(cell)] - double(grid.ground[cell])); };

    // Every cell that is wet now was wetted by an event and gave up its crust then. Should one have
    // been missed, its crust still goes into the lake, but unbooked, so that the check shows it.
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        if (!rewritten(cell) || grid.depth[cell] > 0 || grid.salt[cell] <= 0 || depthOf(cell) <= 0) continue;
        assert(false && "a cell was wetted without an event");
        exchange.lake[lakeOf(cell)].salt += grid.salt[cell] * area;
        grid.salt[cell] = 0;
    }
    // The wet cells of the lakes that took part stand at their lake's level and strength, a cell
    // that dried holds nothing, and a dry cell keeps what crust it has. All others stay as they were.
    bool changed = false;
    std::vector<Tally> after(tree.basins.size());
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        if (!rewritten(cell)) continue;
        const double depth = depthOf(cell);
        if (depth <= 0 && grid.depth[cell] <= 0) continue;
        changed = changed || depth != grid.depth[cell];
        grid.depth[cell] = depth;
        grid.salt[cell] = depth * strengthOf(exchange.lake[lakeOf(cell)]);
        assert(grid.salt[cell] >= 0);
        after[lakeOf(cell)] += {depth * area, grid.salt[cell] * area};
    }
    if (changed) ++ocean.changes;

    // Each body that took part now holds what it held plus what its account booked into it.
    for (size_t id = 1; id < after.size(); ++id) {
        const Tally& turnover = exchange.turnover[id];
        if (!exchange.touched[id] && turnover.water == 0 && turnover.salt == 0) continue;
        const Tally& start = exchange.start[id];
        const Tally& account = exchange.account[id];
        const double water = std::abs(after[id].water - start.water - account.water) /
                             std::max(1.0, std::max(start.water, after[id].water) + turnover.water);
        const double salt = std::abs(after[id].salt - start.salt - account.salt) /
                            std::max(1e-30, std::max(start.salt, after[id].salt) + turnover.salt);
        ocean.unbooked.water = std::max(ocean.unbooked.water, water);
        ocean.unbooked.salt = std::max(ocean.unbooked.salt, salt);
    }
    exchange = Exchange{};
}

} // namespace derivean::sim
