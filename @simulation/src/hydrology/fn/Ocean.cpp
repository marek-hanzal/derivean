#include "hydrology/fn/Ocean.hpp"

#include "hydrology/fn/Strait.hpp"
#include "hydrology/fn/Water.hpp"
#include "planet/type/Planet.hpp"

#include <algorithm>
#include <map>

namespace derivean::sim {
namespace {

constexpr double Gathered = 86400.0;   // s that must gather before the sea is integrated
constexpr double FirstRound = 3600.0;  // s, and longest round: throats in different places take turns
constexpr double LongestRound = 3.15576e7;

} // namespace

int32_t anchorOf(const Ocean& ocean) {
    std::map<int32_t, double> held;
    const auto lakeOf = [&](size_t cell) { return ocean.lakes.lakeOf[size_t(ocean.basins.basinOf[cell])]; };
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell)
        if (ocean.grid.depth[cell] > 0) held[lakeOf(cell)] += ocean.grid.depth[cell] * ocean.basins.area;
    int32_t largest = 0;
    double most = 0;
    for (const auto& entry : held)
        if (entry.second > most) most = entry.second, largest = entry.first;
    int32_t anchor = -1;
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell)
        if (ocean.grid.depth[cell] > 0 && lakeOf(cell) == largest && (anchor < 0 || ocean.grid.depth[cell] > ocean.grid.depth[size_t(anchor)])) anchor = int32_t(cell);
    return anchor;
}

Ocean fillOcean(const Planet& planet, double age) {
    Ocean ocean;
    WaterGrid& grid = ocean.grid;
    grid.width = grid.height = PlanetCells;
    grid.cell = PlanetCell;
    grid.periodic = true;
    grid.ground = planet.elevation.cells;
    grid.kind.assign(grid.cells(), uint8_t(CellKind::Open));
    grid.depth.assign(grid.cells(), 0.0);
    grid.salt.assign(grid.cells(), 0.0);
    ocean.pristineLevel = planet.seaLevel;
    ocean.salinity = MatureSalinity * (1.0 - std::exp(-std::max(0.0, age - RainStart) / SalinityTime));
    if (planet.surfaceWater > 0) {
        for (size_t cell = 0; cell < grid.cells(); ++cell) {
            grid.depth[cell] = std::max(0.0, planet.seaLevel - double(grid.ground[cell]));
            grid.salt[cell] = ocean.salinity * grid.depth[cell];
        }
    }
    ocean.basins = buildBasins(grid);
    ocean.initial = stockOf(grid);
    WaterFlows unused;
    ocean.lakes = settle(grid, ocean.basins, {}, unused);
    ocean.anchor = anchorOf(ocean);
    return ocean;
}

namespace {

// Straits between the same two cells with the same effective sill act as one throat. The throat
// starts and ends on the ground of its ends, so its highest point is never under either of them.
std::vector<Throat> throatsOf(const Ocean& ocean) {
    std::vector<Throat> throats;
    const std::vector<float>& height = ocean.basins.height;
    for (size_t index = 0; index < ocean.straits.size(); ++index) {
        Strait strait = ocean.straits[index];
        strait.sill = std::max({strait.sill, double(height[size_t(strait.a)]), double(height[size_t(strait.b)])});
        const auto same = std::find_if(throats.begin(), throats.end(), [&](const Throat& throat) {
            const Strait& way = throat.way;
            return way.sill == strait.sill && std::min(way.a, way.b) == std::min(strait.a, strait.b) && std::max(way.a, way.b) == std::max(strait.a, strait.b);
        });
        if (same == throats.end()) {
            throats.push_back({strait, {index}});
        } else {
            same->way.area += strait.area;
            same->straits.push_back(index);
        }
    }
    return throats;
}

// Throats in different places may share a sea, and then take turns in rounds. The rounds are fixed
// in time: the first ends an hour after the newest strait was cut, each later one is as long as
// all before it together, up to a year. Fixed rounds make the turns the same however time is fed
// in; taking turns is first order in the round and exact only where the throats share no sea.
double roundEnd(const Ocean& ocean) {
    const double age = ocean.seconds - ocean.opened;
    if (age < FirstRound) return ocean.opened + FirstRound;
    const double length = std::min(LongestRound, FirstRound * std::exp2(std::floor(std::log2(age / FirstRound))));
    return ocean.opened + (std::floor(age / length) + 1.0) * length;
}

// Integrates gathered time: all of it when `all`, else only the rounds that have ended. Each round
// reads the surfaces again, so a throat sees what the one before it left.
void integrate(Ocean& ocean, bool all) {
    const double target = ocean.seconds + ocean.pending;
    const std::vector<Throat> throats = throatsOf(ocean);
    const double before = ocean.seconds;
    Exchange exchange;
    if (!throats.empty()) exchange = openExchange(ocean);
    while (ocean.seconds < target) {
        const double end = throats.size() < 2 ? target : std::min(target, roundEnd(ocean));
        if (end >= target && !all && throats.size() > 1 && roundEnd(ocean) > target) break;
        for (const Throat& throat : throats) flowThrough(ocean, exchange, throat, end - ocean.seconds);
        ocean.seconds = end;
    }
    ocean.pending = target - ocean.seconds;
    if (throats.empty() || ocean.seconds == before) return;
    closeExchange(ocean, exchange);
}

} // namespace

void drainOcean(Ocean& ocean, double seconds) {
    ocean.pending += std::max(0.0, seconds);
    if (ocean.pending >= Gathered) integrate(ocean, false);
}

void settleOcean(Ocean& ocean) { integrate(ocean, true); }

bool openStrait(Ocean& ocean, const Strait& strait) {
    const int32_t cells = int32_t(ocean.grid.cells());
    if (strait.a == strait.b || strait.a < 0 || strait.b < 0 || strait.a >= cells || strait.b >= cells || !(strait.area > 0)) return false;
    // Water poured on a grid that drains across its rim could leave it; a strait must only move water
    // between bodies. The planet grid has no rim.
    const BasinTree& tree = ocean.basins;
    for (const Basin& basin : tree.basins)
        if (basin.parent == 0) return false;
    if (tree.leafOf[size_t(strait.a)] <= 0 || tree.leafOf[size_t(strait.b)] <= 0) return false;
    settleOcean(ocean);
    ocean.straits.push_back(strait);
    ocean.moved.push_back({});
    ocean.opened = ocean.seconds;
    return true;
}

bool seaMoved(const Ocean& ocean) {
    return std::any_of(ocean.moved.begin(), ocean.moved.end(), [](const Tally& moved) { return moved.water != 0; });
}

std::vector<SeaBody> seaBodies(const Ocean& ocean) {
    std::map<int32_t, SeaBody> byLake;
    const double area = ocean.basins.area;
    const auto lakeOf = [&](size_t cell) { return ocean.lakes.lakeOf[size_t(ocean.basins.basinOf[cell])]; };
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell) {
        if (ocean.grid.depth[cell] <= 0) continue;
        SeaBody& body = byLake[lakeOf(cell)];
        body.level = double(ocean.grid.ground[cell]) + ocean.grid.depth[cell];
        body.stock.water += ocean.grid.depth[cell] * area;
        body.stock.salt += ocean.grid.salt[cell] * area;
        ++body.cells;
    }
    for (const Throat& throat : throatsOf(ocean)) {
        const auto wet = [&](int32_t end) { return ocean.grid.depth[size_t(end)] > 0; };
        if (wet(throat.way.a)) ++byLake[lakeOf(size_t(throat.way.a))].straits;
        if (wet(throat.way.b) && !(wet(throat.way.a) && lakeOf(size_t(throat.way.a)) == lakeOf(size_t(throat.way.b))))
            ++byLake[lakeOf(size_t(throat.way.b))].straits;
    }
    if (ocean.anchor >= 0 && ocean.grid.depth[size_t(ocean.anchor)] > 0) byLake[lakeOf(size_t(ocean.anchor))].ocean = true;
    std::vector<SeaBody> bodies;
    for (const auto& entry : byLake) bodies.push_back(entry.second);
    std::stable_sort(bodies.begin(), bodies.end(), [](const SeaBody& a, const SeaBody& b) { return a.stock.water > b.stock.water; });
    return bodies;
}

double seaDepthAt(const Ocean& ocean, int x, int z) {
    return ocean.grid.depth[size_t(wrapIndex(z, PlanetCells)) * PlanetCells + size_t(wrapIndex(x, PlanetCells))];
}

double seaLevelAt(const Ocean& ocean, int x, int z) {
    return double(ocean.grid.ground[size_t(wrapIndex(z, PlanetCells)) * PlanetCells + size_t(wrapIndex(x, PlanetCells))]) + seaDepthAt(ocean, x, z);
}

bool openWaterAt(const Ocean& ocean, int x, int z) { return seaDepthAt(ocean, x, z) > 0; }

double seaLevelNear(const Ocean& ocean, Vec2 planetPosition) {
    const int x = floorToInt(planetPosition.x / PlanetCell), z = floorToInt(planetPosition.z / PlanetCell);
    double deepest = 0, level = WorldFloor;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (seaDepthAt(ocean, x + dx, z + dz) <= deepest) continue;
            deepest = seaDepthAt(ocean, x + dx, z + dz);
            level = seaLevelAt(ocean, x + dx, z + dz);
        }
    }
    return level;
}

} // namespace derivean::sim
