#include "measure/fn/Scout.hpp"

#include "material/constant/Materials.hpp"
#include "measure/fn/Format.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr double Inland = 5000.0;       // m of land behind a shore that must stay gentle
constexpr double GentleRise = 0.03;     // rise over run of that land
constexpr double PlainLength = 10e3;    // m
constexpr double PlainRelief = 150.0;   // m a plain may vary along its line
constexpr double CliffStep = 0.7;       // rise over run between neighbouring samples that only rock holds: a cliff
constexpr double SummitReach = 10e3;    // m within which the two summits of a saddle must stand
constexpr double SummitRise = 300.0;    // m they must stand above the pass
constexpr double PassDrop = 60.0;       // m the ground must fall on both sides across the ridge
constexpr double PassAcross = 4000.0;   // m within which it must do so

constexpr double SummitAlone = 1000.0;  // m within which a cold summit must be the highest ground
constexpr double SummitValley = 3000.0; // m within which its valleys are looked for
constexpr double SummitHeight = 600.0;  // m it must stand above them

constexpr int AxisX[4] = {1, 0, 1, 1}, AxisZ[4] = {0, 1, 1, -1};

bool sea(const Mosaic& mosaic, int x, int z) { return !mosaic.dry(x, z) && !mosaic.lake(x, z); }

// Highest ground met walking from a cell in one direction, and how far away it is; the walk ends
// at water or at the edge. `floor` is the lowest ground crossed before that summit.
struct Climb {
    float top{-1e30f}, floor{1e30f};
    int steps{};
};

Climb climb(const Mosaic& mosaic, int x, int z, int dx, int dz, int reach) {
    Climb found;
    float lowest = mosaic.ground.at(x, z);
    for (int k = 1; k <= reach; ++k) {
        const int nx = x + dx * k, nz = z + dz * k;
        if (nx < 0 || nz < 0 || nx >= mosaic.side() || nz >= mosaic.side() || !mosaic.dry(nx, nz)) break;
        const float here = mosaic.ground.at(nx, nz);
        lowest = std::min(lowest, here);
        if (here > found.top) found = {here, lowest, k};
    }
    return found;
}

// Lowest ground within `reach` cells in one direction.
float fall(const Mosaic& mosaic, int x, int z, int dx, int dz, int reach) {
    float lowest = mosaic.ground.at(x, z);
    for (int k = 1; k <= reach; ++k) {
        const int nx = x + dx * k, nz = z + dz * k;
        if (nx < 0 || nz < 0 || nx >= mosaic.side() || nz >= mosaic.side()) break;
        lowest = std::min(lowest, mosaic.top(nx, nz));
    }
    return lowest;
}

} // namespace

std::vector<ScoutSite> findCoasts(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), reach = cellsIn(mosaic, Inland), thin = std::max(1, reach / 10), stride = cellsIn(mosaic, 500);
    std::vector<float> heights;
    for (int z = reach; z < side - reach; z += stride) {
        for (int x = reach; x < side - reach; x += stride) {
            if (!mosaic.dry(x, z) || !(sea(mosaic, x + 1, z) || sea(mosaic, x - 1, z) || sea(mosaic, x, z + 1) || sea(mosaic, x, z - 1))) continue;
            heights.clear();
            int samples = 0;
            Vec2 land{};
            for (int dz = -reach; dz <= reach; dz += thin)
                for (int dx = -reach; dx <= reach; dx += thin) {
                    if (dx * dx + dz * dz > reach * reach) continue;
                    ++samples;
                    if (!mosaic.dry(x + dx, z + dz)) continue;
                    heights.push_back(float(mosaic.ground.at(x + dx, z + dz) - mosaic.seaLevel));
                    land = land + Vec2{double(dx), double(dz)};
                }
            const double share = double(heights.size()) / samples;
            if (share < 0.3 || share > 0.8) continue;   // a shore has both land and sea in view
            std::sort(heights.begin(), heights.end());
            const double high = heights[size_t(0.9 * double(heights.size() - 1))];
            if (high > GentleRise * Inland) continue;
            ScoutSite site;
            site.at = cellCentre(mosaic, x, z);
            site.ground = mosaic.ground.at(x, z);
            site.level = mosaic.level;
            site.score = share - high / (GentleRise * Inland);
            site.reason = formatText("shore; nine tenths of the land within 5 km lies below %.0f m (%.1f %% rise), land fills %.0f %% of the view", high,
                                     100 * high / Inland, 100 * share);
            // Seen from the water's edge a little along the shore, looking inland.
            const Vec2 inland = normalized(land);
            site.stand = site.at - (inland + perpendicular(inland)) * (0.5 * mosaic.cell + 150.0);
            site.at = site.at + inland * 600.0;
            site.ground = mosaic.top(std::clamp(cellX(mosaic, site.at), 0, side - 1), std::clamp(cellZ(mosaic, site.at), 0, side - 1));
            site.lookUp = 5;
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, 8000);
}

std::vector<ScoutSite> findPlains(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), stride = cellsIn(mosaic, 2000);
    for (int axis = 0; axis < 4; ++axis) {
        const double reachPerStep = mosaic.cell * std::hypot(AxisX[axis], AxisZ[axis]);
        const int count = int(PlainLength / reachPerStep);
        for (int z0 = 0; z0 < side; z0 += stride) {
            for (int x0 = 0; x0 < side; x0 += stride) {
                const int x1 = x0 + AxisX[axis] * count, z1 = z0 + AxisZ[axis] * count;
                if (x1 >= side || z1 < 0 || z1 >= side) continue;
                float low = 1e30f, high = -1e30f, steepest = 0, before = 0;
                bool sound = true;
                double sum = 0;
                for (int k = 0; k <= count && sound; ++k) {
                    const int x = x0 + AxisX[axis] * k, z = z0 + AxisZ[axis] * k;
                    const float here = mosaic.ground.at(x, z);
                    sound = mosaic.dry(x, z) && here > mosaic.seaLevel + 15;
                    if (k > 0) steepest = std::max(steepest, float(std::abs(here - before) / reachPerStep));
                    low = std::min(low, here), high = std::max(high, here), sum += here, before = here;
                }
                if (!sound || high - low > PlainRelief || steepest > CliffStep) continue;
                ScoutSite site;
                site.at = cellCentre(mosaic, x0, z0);
                site.ground = mosaic.ground.at(x0, z0);
                site.level = mosaic.level;
                // Dead level ground shows nothing; a few tens of metres of roll is what ordinary country has.
                site.score = -std::abs((high - low) - 60.0);
                const Vec2 end = cellCentre(mosaic, x1, z1);
                site.reason = formatText("10 km of ordinary country towards X %.0f Z %.0f: relief %.0f m, mean height %.0f m, steepest step %.0f %%, no water", end.x, end.z,
                                         double(high - low), sum / (count + 1), 100.0 * steepest);
                // The viewer starts the fly-through here and looks along the line.
                site.stand = site.at;
                site.at = site.at + normalized(end - site.at) * 2000.0;
                site.ground = mosaic.ground.at(std::clamp(cellX(mosaic, site.at), 0, side - 1), std::clamp(cellZ(mosaic, site.at), 0, side - 1));
                site.lookUp = 2;
                sites.push_back(site);
            }
        }
    }
    return keepApart(std::move(sites), limit, 12000);
}

std::vector<ScoutSite> findSaddles(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), stride = std::max(1, cellsIn(mosaic, 250));
    for (int z = 1; z < side - 1; z += stride) {
        for (int x = 1; x < side - 1; x += stride) {
            if (!mosaic.dry(x, z)) continue;
            const float here = mosaic.ground.at(x, z);
            for (int axis = 0; axis < 4; ++axis) {
                const int dx = AxisX[axis], dz = AxisZ[axis];
                const double step = mosaic.cell * std::hypot(dx, dz);
                const int reach = int(SummitReach / step), across = int(PassAcross / step);
                const Climb a = climb(mosaic, x, z, dx, dz, reach), b = climb(mosaic, x, z, -dx, -dz, reach);
                if (a.top - here < SummitRise || b.top - here < SummitRise) continue;
                // The pass is the lowest point of the ridge line between its two summits.
                if (a.floor < here - 20 || b.floor < here - 20) continue;
                const float dropA = here - fall(mosaic, x, z, -dz, dx, across), dropB = here - fall(mosaic, x, z, dz, -dx, across);
                if (dropA < PassDrop || dropB < PassDrop) continue;
                ScoutSite site;
                site.at = cellCentre(mosaic, x, z);
                site.ground = here;
                site.level = mosaic.level;
                site.score = std::min(a.top, b.top) - here + 0.5 * std::min(dropA, dropB);
                site.reason = formatText("pass at %.0f m between summits of %.0f m (%.1f km) and %.0f m (%.1f km); the ground falls %.0f m and %.0f m to either side",
                                         double(here), double(a.top), a.steps * step / 1000, double(b.top), b.steps * step / 1000, double(dropA), double(dropB));
                // Seen from the lower of the two sides, far enough back for both summits to fit.
                const Vec2 side1 = normalized({double(-dz), double(dx)});
                site.stand = site.at + side1 * (dropA >= dropB ? 2500.0 : -2500.0);
                site.lookUp = 0.4f * (std::min(a.top, b.top) - here);
                sites.push_back(site);
            }
        }
    }
    return keepApart(std::move(sites), limit, 6000);
}

std::vector<ScoutSite> findColdSummits(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), alone = cellsIn(mosaic, SummitAlone), reach = cellsIn(mosaic, SummitValley), thin = std::max(1, reach / 12);
    for (int z = reach; z < side - reach; ++z) {
        for (int x = reach; x < side - reach; ++x) {
            const Material rock = Material(mosaic.rock.at(x, z));
            if ((rock != Material::Granite && rock != Material::Gneiss) || !mosaic.dry(x, z) || mosaic.warmth.at(x, z) > 0) continue;
            const float here = mosaic.ground.at(x, z);
            bool highest = true;
            for (int dz = -alone; dz <= alone && highest; ++dz)
                for (int dx = -alone; dx <= alone && highest; ++dx) highest = mosaic.ground.at(x + dx, z + dz) <= here;
            if (!highest) continue;
            // Valleys around it, and the most ice that lay or lies within their reach.
            float lowest = here, ice = 0, iceAge = 0;
            int lowX = x, lowZ = z;
            for (int dz = -reach; dz <= reach; dz += thin)
                for (int dx = -reach; dx <= reach; dx += thin) {
                    if (mosaic.top(x + dx, z + dz) < lowest) lowest = mosaic.top(x + dx, z + dz), lowX = x + dx, lowZ = z + dz;
                    ice = std::max(ice, mosaic.ice.at(x + dx, z + dz)), iceAge = std::max(iceAge, mosaic.iceAge.at(x + dx, z + dz));
                }
            if (here - lowest < SummitHeight) continue;
            ScoutSite site;
            site.at = cellCentre(mosaic, x, z);
            site.ground = here;
            site.level = mosaic.level;
            site.score = (here - lowest) - 40.0 * mosaic.warmth.at(x, z) + 2.0 * iceAge;
            site.reason = formatText("%s summit at %.0f m, %.0f m over its valleys, annual mean %.1f C; ice within 3 km: %.0f m now, %.0f m at the glacial maximum%s",
                                     definitionOf(rock).name.data(), double(here), double(here - lowest), double(mosaic.warmth.at(x, z)), double(ice), double(iceAge),
                                     iceAge > 20 ? " (look for cirques under the summit)" : "");
            // Seen from the valley floor.
            site.stand = cellCentre(mosaic, lowX, lowZ);
            site.lookUp = 0;
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, 6000);
}

} // namespace derivean::sim
