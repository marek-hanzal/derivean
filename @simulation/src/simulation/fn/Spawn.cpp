#include "simulation/fn/Spawn.hpp"

#include "landscape/fn/Forcing.hpp"
#include "simulation/fn/Reader.hpp"

#include <vector>

namespace derivean::sim {
namespace {

constexpr int PlanetMainland = 200;   // planet cells a landmass needs before its shore counts
constexpr int Hinterland = 4;        // planet cells around a shore cell that are checked for land
constexpr double LandShare = 0.45;   // share of them that must be dry: a coast with country behind it
constexpr int RangeReach = 2;        // planet cells within which a range counts as standing over a place
constexpr double RangeRise = 1500.0; // m such a range must stand above the place
constexpr double ValeCeiling = 1200.0;   // m above the sea a place may lie and still be the low ground under a range
constexpr double RiseForDistance = 0.3;  // m of rise that one more kilometre from the wish is worth
constexpr int RegionOutlook = 14;    // region cells around a shore whose heights make its outlook
constexpr double OutlookForDistance = 9.0;   // m of outlook that one more region cell from the wish is worth

// Size of the landmass every dry planet cell belongs to; -1 for the sea.
struct Landmasses {
    std::vector<int> mass, sizes;
};

Landmasses landmasses(const Planet& planet) {
    const int n = PlanetCells;
    const auto dry = [&](int x, int z) { return planet.elevation.at(x, z) > planet.seaLevel; };
    Landmasses land;
    land.mass.assign(size_t(n) * size_t(n), -1);
    std::vector<int> stack;
    for (int start = 0; start < n * n; ++start) {
        if (land.mass[size_t(start)] >= 0 || !dry(start % n, start / n)) continue;
        const int id = int(land.sizes.size());
        land.sizes.push_back(0);
        stack.push_back(start);
        land.mass[size_t(start)] = id;
        while (!stack.empty()) {
            const int cell = stack.back();
            stack.pop_back();
            ++land.sizes[size_t(id)];
            for (int k = 0; k < 4; ++k) {
                const int x = wrapIndex(cell % n + (k == 0) - (k == 1), n), z = wrapIndex(cell / n + (k == 2) - (k == 3), n);
                const int other = z * n + x;
                if (land.mass[size_t(other)] >= 0 || !dry(x, z)) continue;
                land.mass[size_t(other)] = id;
                stack.push_back(other);
            }
        }
    }
    return land;
}

// A first view should be of living country: not frozen, not scorched, not desert.
bool hospitable(const Planet& planet, size_t cell) {
    return planet.temperature.cells[cell] > 5.0f && planet.temperature.cells[cell] < 24.0f && planet.precipitation.cells[cell] > 0.45f;
}

} // namespace

Vec2 findShore(const Planet& planet, Vec2 near) {
    const int n = PlanetCells;
    const auto dry = [&](int x, int z) { return planet.elevation.at(wrapIndex(x, n), wrapIndex(z, n)) > planet.seaLevel; };

    const Landmasses land = landmasses(planet);
    const std::vector<int>& mass = land.mass;
    const std::vector<int>& sizes = land.sizes;

    // The planet grid is read through a smooth offset; search it where `near` reads it.
    const Vec2 sought = planetPoint(planet.seed, near);
    const int baseX = floorToInt(sought.x / PlanetCell), baseZ = floorToInt(sought.z / PlanetCell);
    double best = 1e30;
    Vec2 found = near;
    // Second round without the climate wish, for worlds that have no such shore.
    for (const bool anyClimate : {false, true}) {
    if (best < 1e30) break;
    for (int dz = -n / 2; dz < n / 2; ++dz) {
        for (int dx = -n / 2; dx < n / 2; ++dx) {
            const int x = baseX + dx, z = baseZ + dz;
            const int id = mass[size_t(wrapIndex(z, n)) * size_t(n) + size_t(wrapIndex(x, n))];
            if (id < 0 || sizes[size_t(id)] < PlanetMainland) continue;
            if (dry(x + 1, z) && dry(x - 1, z) && dry(x, z + 1) && dry(x, z - 1)) continue;
            int land = 0;
            for (int oz = -Hinterland; oz <= Hinterland; ++oz)
                for (int ox = -Hinterland; ox <= Hinterland; ++ox) land += dry(x + ox, z + oz);
            if (land < LandShare * (2 * Hinterland + 1) * (2 * Hinterland + 1)) continue;
            const size_t cell = size_t(wrapIndex(z, n)) * size_t(n) + size_t(wrapIndex(x, n));
            if (planet.surfaceWater > 0 && !hospitable(planet, cell) && !anyClimate) continue;
            const Vec2 center{(x + 0.5) * PlanetCell, (z + 0.5) * PlanetCell};
            const double distance = length(center - sought);
            if (distance < best) {
                best = distance;
                found = center;
            }
        }
    }
    }
    return best >= 1e30 ? near : worldPoint(planet.seed, found);
}

Vec2 findVale(const Planet& planet, Vec2 near) {
    const int n = PlanetCells;
    const Landmasses land = landmasses(planet);
    const auto height = [&](int x, int z) { return double(planet.elevation.at(wrapIndex(x, n), wrapIndex(z, n))) - planet.seaLevel; };
    const Vec2 sought = planetPoint(planet.seed, near);
    const int baseX = floorToInt(sought.x / PlanetCell), baseZ = floorToInt(sought.z / PlanetCell);
    double best = -1e30;
    Vec2 found = near;
    for (int dz = -n / 2; dz < n / 2; ++dz) {
        for (int dx = -n / 2; dx < n / 2; ++dx) {
            const int x = baseX + dx, z = baseZ + dz;
            const size_t cell = size_t(wrapIndex(z, n)) * size_t(n) + size_t(wrapIndex(x, n));
            const double own = height(x, z);
            if (own <= 0 || own > ValeCeiling || land.sizes[size_t(land.mass[cell])] < PlanetMainland || !hospitable(planet, cell)) continue;
            double range = 0;
            for (int oz = -RangeReach; oz <= RangeReach; ++oz)
                for (int ox = -RangeReach; ox <= RangeReach; ++ox) range = std::max(range, height(x + ox, z + oz));
            if (range - own < RangeRise) continue;
            const Vec2 center{(x + 0.5) * PlanetCell, (z + 0.5) * PlanetCell};
            const double score = (range - own) - RiseForDistance * length(center - sought) / 1000.0;
            if (score > best) {
                best = score;
                found = center;
            }
        }
    }
    return best <= -1e30 ? findShore(planet, near) : worldPoint(planet.seed, found);
}

Vec2 findLanding(World& world, Vec2 near) {
    Vec2 at = findVale(world.planet, near);
    constexpr int Reach[4] = {0, 96, 160, 160};          // cells searched around the current answer
    constexpr float Freeboard[4] = {0, 3.0f, 1.0f, 0.4f}; // m a cell must stand above the water to count as dry
    constexpr int Mainland[4] = {0, 300, 600, 40};        // cells a piece of land needs before its shore counts
    for (int step = int(Level::Region); step <= int(Level::Site); ++step) {
        const Level level = Level(step);
        const double cell = specOf(level).cell;
        const int reach = Reach[step];
        const TileKey low = tileAt(level, {at.x - reach * cell, at.z - reach * cell}), high = tileAt(level, {at.x + reach * cell, at.z + reach * cell});
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x) world.atlas.request({level, x, z});
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x) world.atlas.require({level, x, z});
        TileReader reader{world.atlas, level};
        const float sea = float(world.planet.seaLevel);
        // A dry cell carries a water value below its ground; only the sea can then be its water level.
        const auto water = [&](Vec2 p, SurfaceSample& sample) {
            return readSurface(reader, p, sample) && sample.water > sample.ground ? std::max(sample.water, sea) : sea;
        };
        const Vec2 centre{(std::floor(at.x / cell) + 0.5) * cell, (std::floor(at.z / cell) + 0.5) * cell};

        // Dry cells of the search square and the size of the piece of land each belongs to, so the
        // answer is the shore of real country and not of a rock in the sea.
        const int side = 2 * reach + 1;
        std::vector<uint8_t> dry(size_t(side) * size_t(side), 0);
        std::vector<float> ground(dry.size(), 0.0f);
        for (int dz = -reach; dz <= reach; ++dz)
            for (int dx = -reach; dx <= reach; ++dx) {
                SurfaceSample sample;
                const size_t i = size_t(dz + reach) * size_t(side) + size_t(dx + reach);
                dry[i] = water({centre.x + dx * cell, centre.z + dz * cell}, sample) + Freeboard[step] <= sample.ground;
                ground[i] = sample.ground;
            }
        std::vector<int> piece(dry.size(), -1), sizes, stack;
        for (int start = 0; start < int(dry.size()); ++start) {
            if (!dry[size_t(start)] || piece[size_t(start)] >= 0) continue;
            piece[size_t(start)] = int(sizes.size());
            sizes.push_back(0);
            stack.push_back(start);
            while (!stack.empty()) {
                const int cellIndex = stack.back();
                stack.pop_back();
                ++sizes.back();
                const int x = cellIndex % side, z = cellIndex / side;
                for (int k = 0; k < 8; k += 2) {
                    const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                    if (nx < 0 || nz < 0 || nx >= side || nz >= side) continue;
                    const int other = nz * side + nx;
                    if (!dry[size_t(other)] || piece[size_t(other)] >= 0) continue;
                    piece[size_t(other)] = piece[size_t(cellIndex)];
                    stack.push_back(other);
                }
            }
        }
        // The region decides the scenery: among its shores the one with the most country standing
        // over it wins, a farther one only if it gains enough. Finer levels just find the water's edge.
        const auto outlook = [&](int x, int z) {
            if (level != Level::Region) return 0.0;
            float highest = ground[size_t(z) * size_t(side) + size_t(x)];
            for (int oz = -RegionOutlook; oz <= RegionOutlook; oz += 2)
                for (int ox = -RegionOutlook; ox <= RegionOutlook; ox += 2) {
                    const int nx = std::clamp(x + ox, 0, side - 1), nz = std::clamp(z + oz, 0, side - 1);
                    highest = std::max(highest, ground[size_t(nz) * size_t(side) + size_t(nx)]);
                }
            return double(highest - ground[size_t(z) * size_t(side) + size_t(x)]) / OutlookForDistance;
        };
        double best = 1e30;
        for (int z = 1; z < side - 1; ++z) {
            for (int x = 1; x < side - 1; ++x) {
                const size_t i = size_t(z) * size_t(side) + size_t(x);
                if (!dry[i] || sizes[size_t(piece[i])] < Mainland[step]) continue;
                if (dry[i + 1] && dry[i - 1] && dry[i + size_t(side)] && dry[i - size_t(side)]) continue;
                const double cost = std::hypot(x - reach, z - reach) - outlook(x, z);
                if (cost >= best) continue;
                best = cost;
                at = {centre.x + (x - reach) * cell, centre.z + (z - reach) * cell};
            }
        }
    }
    return at;
}

} // namespace derivean::sim
