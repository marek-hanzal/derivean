#include "measure/fn/Scout.hpp"

#include "landscape/type/Terrain.hpp"
#include "measure/fn/Format.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr double TrunkFlow = 1e8;        // m³/yr of a trunk stream, about 3 m³/s
constexpr double ValleyWidth = 3000.0;   // m within which the valley walls are looked for
constexpr double ValleyWall = 150.0;     // m both walls must rise
constexpr double GorgeFlow = 3e6;        // m³/yr; a gorge needs a stream, not a river
constexpr double GorgeDepth = 100.0;     // m both rims must stand above the floor
constexpr double GorgeReach = 1000.0;    // m within which the rims and the level ground must lie
constexpr double LevelRelief = 35.0;     // m of relief over 500 m that still counts as level
constexpr double BasinArea = 0.05e6;     // m² a lake must cover
constexpr double BasinHeight = 800.0;    // m its surface must reach
constexpr double BasinOutlet = 3e5;      // m³/yr that must leave it over its rim: a lake that only evaporates has no outlet
constexpr double BasinRise = 100.0;      // m the ground must rise around it
constexpr double BasinReach = 1500.0;    // m from its shore within which it must do so


// Direction of flow over the next few cells, in cells; zero where the path ends at once.
Vec2 heading(const Mosaic& mosaic, int x, int z) {
    int nx = x, nz = z;
    for (int k = 0; k < 4; ++k) {
        const uint8_t to = mosaic.flowTo.at(nx, nz);
        if (to > 7 || !mosaic.contains(nx + NeighborX[to], nz + NeighborZ[to])) break;
        nx += NeighborX[to], nz += NeighborZ[to];
    }
    return nx == x && nz == z ? Vec2{0, 0} : normalized({double(nx - x), double(nz - z)});
}

// Ground along a walk from a cell in a direction given in cells, between two distances in metres:
// its highest and lowest point.
struct Span {
    float low{1e30f}, high{-1e30f};
    bool whole{true};
};

Span span(const Mosaic& mosaic, int x, int z, Vec2 direction, double from, double to) {
    Span found;
    for (double d = from; d <= to; d += mosaic.cell) {
        const int nx = x + int(std::lround(direction.x * d / mosaic.cell)), nz = z + int(std::lround(direction.z * d / mosaic.cell));
        if (!mosaic.contains(nx, nz)) {
            found.whole = false;
            break;
        }
        found.low = std::min(found.low, mosaic.top(nx, nz)), found.high = std::max(found.high, mosaic.top(nx, nz));
    }
    return found;
}

} // namespace

std::vector<ScoutSite> findValleys(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), stride = std::max(1, cellsIn(mosaic, 300));
    for (int z = 0; z < side; z += stride) {
        for (int x = 0; x < side; x += stride) {
            if (mosaic.discharge.at(x, z) < TrunkFlow || mosaic.flooded(x, z)) continue;
            const Vec2 flow = heading(mosaic, x, z);
            if (length(flow) < 0.5) continue;
            const float floor = mosaic.ground.at(x, z);
            const Vec2 across = perpendicular(flow);
            const Span left = span(mosaic, x, z, across, mosaic.cell, ValleyWidth), right = span(mosaic, x, z, across * -1.0, mosaic.cell, ValleyWidth);
            const double wall = std::min(left.high, right.high) - floor;
            if (!left.whole || !right.whole || wall < ValleyWall) continue;
            ScoutSite site;
            site.at = cellCentre(mosaic, x, z);
            site.ground = floor;
            site.level = mosaic.level;
            site.score = wall * std::log10(mosaic.discharge.at(x, z));
            site.reason = formatText("river of %.1f m3/s on a floor at %.0f m; walls rise %.0f m and %.0f m within 3 km", mosaic.discharge.at(x, z) / 31557600.0,
                                     double(floor), double(left.high - floor), double(right.high - floor));
            // Seen from the valley floor downstream, looking up the valley.
            site.stand = site.at + flow * 900.0 + across * (1.5 * mosaic.cell);
            site.lookUp = float(0.35 * wall);
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, 8000);
}

std::vector<ScoutSite> findGorges(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side();
    // A region cell is as wide as the whole reach, so there the cut only shows as a cell lying below level neighbours.
    const bool coarse = mosaic.cell > 0.5 * GorgeReach;
    const double depthWanted = coarse ? 0.3 * GorgeDepth : GorgeDepth, levelWanted = coarse ? 3 * LevelRelief : LevelRelief;
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            if (mosaic.discharge.at(x, z) < GorgeFlow || mosaic.flooded(x, z)) continue;
            const Vec2 flow = heading(mosaic, x, z);
            if (length(flow) < 0.5) continue;
            const float floor = mosaic.ground.at(x, z);
            const Vec2 across = perpendicular(flow);
            const double near = coarse ? mosaic.cell : 0.5 * GorgeReach, far = coarse ? 2 * mosaic.cell : GorgeReach;
            const Span left = span(mosaic, x, z, across, near, far), right = span(mosaic, x, z, across * -1.0, near, far);
            if (!left.whole || !right.whole) continue;
            const double depth = std::min(left.low, right.low) - floor;
            const double relief = std::max(left.high - left.low, right.high - right.low);
            if (depth < depthWanted || relief > levelWanted) continue;
            ScoutSite site;
            site.at = cellCentre(mosaic, x, z);
            site.ground = floor;
            site.level = mosaic.level;
            site.score = depth - 2 * relief;
            site.reason = formatText("cut %.0f m deep with a stream of %.2f m3/s; the ground %.0f to %.0f m to either side is level within %.0f m", depth,
                                     mosaic.discharge.at(x, z) / 31557600.0, near, far, relief);
            // Seen from the level ground at the rim, looking down into the cut.
            site.stand = site.at + across * (left.high - left.low <= right.high - right.low ? near : -near);
            site.lookUp = 0;
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, 5000);
}

std::vector<ScoutSite> findBasins(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), reach = cellsIn(mosaic, BasinReach);
    std::vector<int32_t> seen(size_t(side) * size_t(side), 0);   // number of the lake a cell belongs to, from 1
    std::vector<int32_t> lake;
    int number = 0;
    for (int start = 0; start < side * side; ++start) {
        if (seen[size_t(start)] || !mosaic.lake(start % side, start / side)) continue;
        lake.assign(1, start);
        seen[size_t(start)] = ++number;
        double sumX = 0, sumZ = 0;
        float deepest = 0, outflow = 0;
        const float surface = mosaic.water.at(start % side, start / side);
        int lowX = side, lowZ = side, highX = 0, highZ = 0;
        for (size_t next = 0; next < lake.size(); ++next) {
            const int x = lake[next] % side, z = lake[next] / side;
            sumX += x, sumZ += z;
            deepest = std::max(deepest, mosaic.water.at(x, z) - mosaic.ground.at(x, z));
            lowX = std::min(lowX, x), lowZ = std::min(lowZ, z), highX = std::max(highX, x), highZ = std::max(highZ, z);
            for (int k = 0; k < 8; k += 2) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (!mosaic.contains(nx, nz)) continue;
                // Water leaves over dry ground at the level of the lake whose own flow does not turn back into it.
                if (!mosaic.flooded(nx, nz) && mosaic.ground.at(nx, nz) <= surface + 0.5f) {
                    const uint8_t to = mosaic.flowTo.at(nx, nz);
                    const int tx = to > 7 ? nx : nx + NeighborX[to], tz = to > 7 ? nz : nz + NeighborZ[to];
                    if (to <= 7 && mosaic.contains(tx, tz) && !mosaic.lake(tx, tz)) outflow = std::max(outflow, mosaic.discharge.at(nx, nz));
                }
                if (seen[size_t(nz * side + nx)] || !mosaic.lake(nx, nz)) continue;
                seen[size_t(nz * side + nx)] = number;
                lake.push_back(nz * side + nx);
            }
        }
        const double area = double(lake.size()) * mosaic.cell * mosaic.cell;
        const float level = surface;
        if (area < BasinArea || level < BasinHeight || outflow < BasinOutlet) continue;
        // Highest ground within reach of the lake's bounding box, and where it stands.
        float rim = level;
        int rimX = lowX, rimZ = lowZ;
        for (int z = std::max(0, lowZ - reach); z <= std::min(side - 1, highZ + reach); ++z)
            for (int x = std::max(0, lowX - reach); x <= std::min(side - 1, highX + reach); ++x)
                if (mosaic.ground.at(x, z) > rim) rim = mosaic.ground.at(x, z), rimX = x, rimZ = z;
        if (rim - level < BasinRise) continue;
        ScoutSite site;
        const Vec2 centre = cellCentre(mosaic, int(sumX / double(lake.size())), int(sumZ / double(lake.size())));
        site.at = centre;
        site.ground = level;
        site.level = mosaic.level;
        // A tarn under a wall shows as well as an inland sea does; beyond 2 km² more water adds nothing.
        site.score = (rim - level) * std::sqrt(std::min(area, 2e6) / 1e6);
        site.reason = formatText("lake of %.2f km2 at %.0f m, up to %.0f m deep, spilling %.2f m3/s; ground rises %.0f m within 1.5 km of it", area / 1e6,
                                 double(level), double(deepest), outflow / 31557600.0, double(rim - level));
        // Seen from the shore opposite the highest ground, looking across the water at it: the
        // viewer stands on the first dry ground past the last water on the way out from the middle.
        const Vec2 away = normalized(centre - cellCentre(mosaic, rimX, rimZ));
        double shore = mosaic.cell;
        for (double d = mosaic.cell; d < (highX - lowX + highZ - lowZ + 4) * mosaic.cell; d += mosaic.cell) {
            const Vec2 here = centre + away * d;
            if (!mosaic.contains(cellX(mosaic, here), cellZ(mosaic, here))) break;
            if (seen[size_t(cellZ(mosaic, here) * side + cellX(mosaic, here))] == number) shore = d + mosaic.cell;
        }
        site.stand = centre + away * (shore + mosaic.cell);
        site.lookUp = 0;
        sites.push_back(site);
    }
    return keepApart(std::move(sites), limit, 3000);
}

} // namespace derivean::sim
