#include "intervention/fn/Scenario.hpp"

#include "hydrology/fn/Ocean.hpp"
#include "hydrology/fn/Water.hpp"

#include <limits>

namespace derivean::sim {
namespace {

// Heaviest-laden stream cell of a tile on dry ground, restricted to cells in the middle of a child
// tile with even keys that does not touch the tile's edge; -1 when there is none.
int streamCell(const Tile& tile, double least, double most, float clearance, float relief) {
    int best = -1;
    for (int cell = 0; cell < TileCells * TileCells; ++cell) {
        const size_t i = size_t(cell);
        const float ground = groundOf(tile.now, i);
        if (ground < tile.seaLevel + clearance || tile.water[i] > ground || tile.discharge[i] < least || tile.discharge[i] > most) continue;
        const int x = cell % TileCells, z = cell / TileCells;
        const int childX = tile.key.x * Refinement + x / Refinement, childZ = tile.key.z * Refinement + z / Refinement;
        if ((childX & 1) || (childZ & 1) || x < Refinement || z < Refinement || x >= TileCells - Refinement || z >= TileCells - Refinement) continue;
        if (x % Refinement < 5 || x % Refinement > 10 || z % Refinement < 5 || z % Refinement > 10) continue;
        float low = ground, high = ground;
        for (int dz = -2; dz <= 2; ++dz)
            for (int dx = -2; dx <= 2; ++dx) {
                const float other = groundOf(tile.now, size_t(z + dz) * TileCells + size_t(x + dx));
                low = std::min(low, other), high = std::max(high, other);
            }
        if (high - low < relief) continue;
        if (best < 0 || tile.load[i] > tile.load[size_t(best)]) best = cell;
    }
    return best;
}

} // namespace

StreamSite findStream(World& world) {
    struct Candidate {
        float flow;
        int x, z;
    };
    std::vector<Candidate> candidates;
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x)
            if (world.planet.elevation.at(x, z) > world.planet.seaLevel + 100 && !((x / 4) & 1) && !((z / 4) & 1))
                candidates.push_back({world.backdrop.discharge.at(x, z), x, z});
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) { return a.flow > b.flow; });
    TileKey tried{Level::Region, std::numeric_limits<int32_t>::max(), 0};
    int regions = 0;
    for (const Candidate& candidate : candidates) {
        Vec2 at = planetCellCenter(candidate.x, candidate.z);
        if (tileAt(Level::Region, at) == tried) continue;
        tried = tileAt(Level::Region, at);
        if (++regions > 6) break;
        bool found = true;
        for (const Level level : {Level::Region, Level::Local}) {
            const TileKey key = tileAt(level, at);
            const std::shared_ptr<const Tile> tile = world.atlas.require(key);
            const int cell = level == Level::Region ? streamCell(*tile, 3e6, 3e9, 30.0f, 60.0f) : streamCell(*tile, 3e5, 3e7, 5.0f, 12.0f);
            found = cell >= 0;
            if (!found) break;
            at = {(key.x * double(TileCells) + cell % TileCells + 0.5) * specOf(level).cell, (key.z * double(TileCells) + cell / TileCells + 0.5) * specOf(level).cell};
        }
        if (!found) continue;
        const StreamSite site{true, tileAt(Level::Site, at), at};
        const LiveSet& set = openSite(world, site);
        if (cascadePlace(set).anchor < 0 || diversionPlace(set).anchor < 0) continue;
        forgetInterventions(world);
        return site;
    }
    forgetInterventions(world);
    return {};
}

const LiveSet& openSite(World& world, const StreamSite& site) {
    forgetInterventions(world);
    intervene(world, watch(site.at));
    advance(world, SiteWatch);
    syncLive(world);
    return setOf(world);
}

double surfaceAt(const LiveSet& set, int32_t cell) { return double(set.grid.ground[size_t(cell)]) + set.grid.depth[size_t(cell)]; }

double waterIn(const LiveSet& set, int x0, int x1, int z0, int z1) {
    if (x0 > x1) std::swap(x0, x1);
    double volume = 0;
    for (int z = z0; z <= z1; ++z)
        for (int x = x0; x <= x1; ++x) volume += set.grid.depth[size_t(z) * size_t(set.grid.width) + size_t(x)];
    return volume * set.grid.cell * set.grid.cell;
}

Vec2 centreOf(const LiveSet& set, int32_t cell) {
    const double size = set.grid.cell;
    return {(double(set.originX) * TileCells + cell % set.grid.width + 0.5) * size, (double(set.originZ) * TileCells + cell / set.grid.width + 0.5) * size};
}

int32_t streamAnchor(const LiveSet& set, int west, int east, int north, int south, bool dry) {
    const WaterGrid& grid = set.grid;
    int32_t best = -1;
    double nearest = 1e30;
    for (int z = EditClear + north; z < grid.height - EditClear - south; ++z) {
        for (int x = EditClear + west; x < grid.width - EditClear - east; ++x) {
            const size_t cell = size_t(z) * size_t(grid.width) + size_t(x);
            if (grid.discharge[cell] < StreamFlow || (dry && grid.depth[cell] > 0)) continue;
            const double away = std::hypot(x - 0.5 * grid.width, z - 0.5 * grid.height);
            if (away >= nearest) continue;
            nearest = away;
            best = int32_t(cell);
        }
    }
    return best;
}

std::vector<int32_t> courseFrom(const LiveSet& set, int32_t cell, int steps) {
    const BasinTree& tree = set.basins;
    std::vector<int32_t> course;
    for (int step = 0; step < steps && cell >= 0 && set.grid.kindOf(cell) == CellKind::Open; ++step) {
        course.push_back(cell);
        int32_t next = tree.receiver[size_t(cell)];
        if (next < 0 && tree.basinOf[size_t(cell)]) {
            // Across the lake the pit lies in, to where its overflow comes down.
            const int32_t top = set.lakes.lakeOf[size_t(tree.basinOf[size_t(cell)])];
            next = set.lakes.full[size_t(top)] ? tree.basins[size_t(top)].entry : -1;
        }
        cell = next;
    }
    return course;
}

Vec2 arrivesFrom(const LiveSet& set, int32_t cell) {
    const WaterGrid& grid = set.grid;
    int32_t at = cell;
    for (int step = 0; step < 12; ++step) {
        int32_t feeder = -1;
        for (int k = 0; k < 8; ++k) {
            const int32_t other = grid.neighbour(at, k);
            if (other < 0 || set.basins.receiver[size_t(other)] != at) continue;
            if (feeder < 0 || grid.discharge[size_t(other)] > grid.discharge[size_t(feeder)]) feeder = other;
        }
        if (feeder < 0) break;
        at = feeder;
    }
    return at == cell ? Vec2{0, 0} : normalized(centreOf(set, at) - centreOf(set, cell));
}

LakeState lakeAt(const LiveSet& set, int32_t cell) {
    const WaterGrid& grid = set.grid;
    const BasinTree& tree = set.basins;
    LakeState state;
    state.level = grid.ground[size_t(cell)];
    const int32_t basin = tree.basinOf[size_t(cell)];
    if (!basin || grid.depth[size_t(cell)] <= 0) return state;
    const int32_t top = set.lakes.lakeOf[size_t(basin)];
    state.level = set.lakes.level[size_t(top)];
    state.full = set.lakes.full[size_t(top)];
    for (size_t other = 0; other < grid.cells(); ++other) {
        if (grid.depth[other] <= 0 || !tree.basinOf[other] || set.lakes.lakeOf[size_t(tree.basinOf[other])] != top) continue;
        state.volume += grid.depth[other] * tree.area;
        ++state.cells;
    }
    return state;
}

double valueOf(const ScenarioReport& report, size_t stage, const std::string& measure) {
    for (size_t i = 0; i < report.measures.size(); ++i)
        if (report.measures[i] == measure) return report.stages[stage].values[i];
    return std::numeric_limits<double>::quiet_NaN();
}

void forgetInterventions(World& world) {
    const double era = world.interventions.calendar.era;
    world.interventions = Interventions{};
    world.interventions.calendar.era = era;
    world.ocean = fillOcean(world.planet, world.options.age);
}

WorksPlace cascadePlace(const LiveSet& set) {
    WorksPlace place;
    double nearest = 1e30;
    for (int z = EditClear + 12; z < set.grid.height - EditClear - 12; ++z) {
        for (int x = EditClear + 34; x < set.grid.width - EditClear - 34; ++x) {
            const int32_t cell = z * set.grid.width + x;
            const double middle = std::hypot(x - 0.5 * set.grid.width, z - 0.5 * set.grid.height);
            if (middle >= nearest || set.grid.depth[size_t(cell)] > 0 || set.grid.discharge[size_t(cell)] < StreamFlow) continue;
            const int side = arrivesFrom(set, cell).x > 0 ? -1 : 1;
            if (waterIn(set, x - 9 * side, x + 33 * side, z - 11, z + 11) > 100.0) continue;
            nearest = middle, place = {cell, side};
        }
    }
    return place;
}

IntakePlace diversionPlace(const LiveSet& set) {
    constexpr int Room = 12;   // cells the channel may wander from its ends
    const WaterGrid& grid = set.grid;
    const auto inRange = [&](int x, int z) { return x >= EditClear + Room && z >= EditClear + Room && x < grid.width - EditClear - Room && z < grid.height - EditClear - Room; };
    IntakePlace place;
    double best = 1e30;
    for (int z = 0; z < grid.height; ++z) {
        for (int x = 0; x < grid.width; ++x) {
            const int32_t cell = z * grid.width + x;
            const double middle = std::hypot(x - 0.5 * grid.width, z - 0.5 * grid.height);
            if (!inRange(x, z) || middle >= best || grid.depth[size_t(cell)] > 0 || grid.discharge[size_t(cell)] < StreamFlow) continue;
            const std::vector<int32_t> course = courseFrom(set, cell, 48);
            bool oneBed = course.size() >= 8;
            for (size_t i = 1; oneBed && i < 8; ++i)
                oneBed = grid.depth[size_t(course[i])] <= 0 && grid.discharge[size_t(course[i])] < 1.5f * grid.discharge[size_t(cell)];
            if (!oneBed) continue;
            const double sill = std::min(surfaceAt(set, cell) - 0.15, double(grid.ground[size_t(course[1])]) - 0.1);
            const Vec2 from = centreOf(set, cell);
            int32_t found = -1;
            double nearest = 1e30;
            for (int oz = z - 50; oz <= z + 50; ++oz) {
                for (int ox = x - 50; ox <= x + 50; ++ox) {
                    const int32_t other = oz * grid.width + ox;
                    const double away = inRange(ox, oz) ? length(centreOf(set, other) - from) : 0.0;
                    if (away < 60 || away > 200 || surfaceAt(set, other) > sill - 0.5) continue;
                    const double rank = away + (grid.discharge[size_t(other)] >= StreamFlow ? 0.0 : 1000.0);
                    if (rank < nearest) nearest = rank, found = other;
                }
            }
            if (found < 0) continue;
            best = middle, place = {cell, found, sill, course};
        }
    }
    return place;
}

} // namespace derivean::sim
