#include "hydrology/fn/Ocean.hpp"

#include "hydrology/fn/Water.hpp"
#include "planet/type/Planet.hpp"

#include <map>

namespace derivean::sim {
namespace {

constexpr double Gathered = 86400.0;   // s that must gather before the sea is integrated
constexpr double FirstRound = 3600.0;  // s, and longest round: openings in different places take turns
constexpr double LongestRound = 3.15576e7;

// One opening for `seconds`. Head h over the mouth falls as dh/dt = −C·A·√(2gh) / (wet area), and
// the wet area is constant between the heights of two cells, so √h falls linearly in each such
// slice. When the surface reaches the saddle a basin's children met at, the basin is empty and the
// child the opening lies in carries on alone: the other child keeps what it holds.
void drainThrough(Ocean& ocean, const VoidOpening& opening, double seconds) {
    const BasinTree& tree = ocean.basins;
    const double mouth = std::max(opening.mouth, double(tree.height[size_t(opening.cell)]));
    const double pull = OrificeCoefficient * opening.area * std::sqrt(2 * Gravity);
    for (double left = seconds; left > 0;) {
        // The surface over the opening belongs to the topmost basin above it holding water of its own.
        int32_t holder = -1;
        for (int32_t id = tree.basinOf[size_t(opening.cell)]; id > 0; id = tree.basins[size_t(id)].parent)
            if (ocean.lakes.own[size_t(id)] > 0) holder = id;
        if (holder < 0) return;
        const Basin& basin = tree.basins[size_t(holder)];
        const auto heightOf = [&](int32_t member) { return double(tree.height[size_t(tree.members[size_t(basin.first + member)])]); };
        double level = levelOf(tree, holder, ocean.lakes.own[size_t(holder)]);
        if (level <= basin.base) {
            // Less than the level can show is left above the saddle; the basin is empty.
            ocean.lakes.own[size_t(holder)] = 0;
            continue;
        }
        const double floor = std::max(basin.base, mouth);
        if (level <= floor) return;
        int32_t wet = 0;
        while (wet < basin.count && heightOf(wet) < level) ++wet;
        while (left > 0 && level > floor) {
            const double bottom = std::max(floor, wet > 0 ? heightOf(wet - 1) : basin.base);
            const double pace = pull / (2.0 * double(basin.below + wet) * tree.area);   // fall of √h per second
            const double from = std::sqrt(level - mouth), to = std::sqrt(bottom - mouth);
            if ((from - to) / pace >= left) {
                const double root = from - left * pace;
                level = mouth + root * root;
                left = 0;
                break;
            }
            left -= (from - to) / pace;
            level = bottom;
            if (wet > 0 && heightOf(wet - 1) >= level) --wet;
        }
        ocean.lakes.own[size_t(holder)] = level <= basin.base ? 0.0 : std::min(ocean.lakes.own[size_t(holder)], volumeOf(tree, holder, level));
        if (level > basin.base) return;
    }
}

} // namespace

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
    return ocean;
}

namespace {

// Openings side by side act as one throat.
std::vector<VoidOpening> throatsOf(const Ocean& ocean) {
    std::vector<VoidOpening> throats;
    for (const VoidOpening& opening : ocean.openings) {
        const double mouth = std::max(opening.mouth, double(ocean.basins.height[size_t(opening.cell)]));
        const auto same = std::find_if(throats.begin(), throats.end(), [&](const VoidOpening& other) { return other.cell == opening.cell && other.mouth == mouth; });
        if (same == throats.end()) throats.push_back({opening.cell, opening.area, mouth});
        else same->area += opening.area;
    }
    return throats;
}

// Throats in different places may share a sea, and then take turns in rounds. The rounds are fixed
// in time: the first ends an hour after the newest opening was made, each later one is as long as
// all before it together, up to a year. Fixed rounds make the turns the same however time is fed
// in; taking turns is first order in the round and exact only where the throats share no sea.
double roundEnd(const Ocean& ocean) {
    const double age = ocean.seconds - ocean.opened;
    if (age < FirstRound) return ocean.opened + FirstRound;
    const double length = std::min(LongestRound, FirstRound * std::exp2(std::floor(std::log2(age / FirstRound))));
    return ocean.opened + (std::floor(age / length) + 1.0) * length;
}

// Integrates gathered time: all of it when `all`, else only the rounds that have ended.
void integrate(Ocean& ocean, bool all) {
    const double target = ocean.seconds + ocean.pending;
    const std::vector<VoidOpening> throats = throatsOf(ocean);
    const double before = ocean.seconds;
    while (ocean.seconds < target) {
        const double end = throats.size() < 2 ? target : std::min(target, roundEnd(ocean));
        if (end >= target && !all && throats.size() > 1 && roundEnd(ocean) > target) break;
        for (const VoidOpening& throat : throats) drainThrough(ocean, throat, end - ocean.seconds);
        ocean.seconds = end;
    }
    ocean.pending = target - ocean.seconds;
    if (throats.empty() || ocean.seconds == before) return;
    refreshLakes(ocean.basins, ocean.lakes);
    // Water leaves mixed, so what stays keeps its strength. A cell whose depth did not change is
    // left exactly as it was: a sea nothing drains must not drift by rounding.
    WaterGrid& grid = ocean.grid;
    const double area = ocean.basins.area;
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        const double was = grid.depth[cell];
        const double now = std::max(0.0, ocean.lakes.level[size_t(ocean.basins.basinOf[cell])] - double(grid.ground[cell]));
        if (was <= 0 || now >= was * (1.0 - 1e-12)) continue;
        const double kept = grid.salt[cell] * now / was;
        ocean.escaped.water += (was - now) * area;
        ocean.escaped.salt += (grid.salt[cell] - kept) * area;
        grid.depth[cell] = now;
        grid.salt[cell] = kept;
    }
}

} // namespace

void drainOcean(Ocean& ocean, double seconds) {
    ocean.pending += std::max(0.0, seconds);
    if (ocean.pending >= Gathered) integrate(ocean, false);
}

void settleOcean(Ocean& ocean) { integrate(ocean, true); }

void openVoid(Ocean& ocean, const VoidOpening& opening) {
    settleOcean(ocean);
    ocean.openings.push_back(opening);
    ocean.opened = ocean.seconds;
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
    for (const VoidOpening& opening : ocean.openings)
        if (ocean.grid.depth[size_t(opening.cell)] > 0) byLake[lakeOf(size_t(opening.cell))].draining = true;
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
