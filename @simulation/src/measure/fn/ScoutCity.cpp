#include "measure/fn/Scout.hpp"

#include "landscape/fn/Forcing.hpp"
#include "material/constant/Materials.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Format.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr float CityThickness = 60.0f;   // m of sandstone in one run
constexpr float CityDepth = 100.0f;      // m below the rock surface within which that run must lie
constexpr double CityDip = 0.0524;       // rise over run of beds dipping 3°
constexpr double CityReliefLow = 80.0, CityReliefHigh = 300.0;   // m within a kilometre: dissected, not a wall

// The thickest unbroken run of sandstone within CityDepth of the rock surface: how much of it lies
// within that depth, how far below the rock surface it starts, and the height of its base. The
// base is followed below CityDepth, since erosion has cut the top of a run that crops out but not
// its base. The layering is read at the point itself, as the voxels read it.
struct SandstoneRun {
    float thickness{}, starts{}, base{};
};

SandstoneRun sandstoneUnder(const Planet& planet, Vec2 at, float datum, float rock) {
    const StrataColumn column = strataAt(planet, planetPoint(planet.seed, at));
    const float fold = foldAt(planet.seed, column, column.material);
    SandstoneRun best, run;
    const float surface = std::max(0.0f, datum - rock), end = surface + CityDepth;
    for (float depth = surface; depth < end + 4 * CityDepth;) {
        const Bed bed = bedAt(planet.seed, column, unitAt(column, depth, fold), depth, column.material);
        const float bottom = std::max(bed.bottom, depth + 0.5f);
        if (bed.rock == Material::Sandstone) {
            if (run.thickness == 0) run.starts = depth - surface;
            if (depth < end) run.thickness += std::min(bottom, end) - depth;
            run.base = datum - bottom;
            if (run.thickness > best.thickness || (run.starts == best.starts && run.thickness == best.thickness)) best = run;
        } else {
            if (depth >= end) break;
            run = {};
        }
        depth = bottom;
    }
    return best;
}

} // namespace

std::vector<ScoutSite> findRockCities(const Mosaic& mosaic, const Planet& planet, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), reach = cellsIn(mosaic, 500), apart = cellsIn(mosaic, 100), stride = cellsIn(mosaic, 250);
    // A region cell hides a rock city inside its mean, so there only the rock and the relief around it are asked;
    // thickness and dip are settled on the local cells of the places it names.
    const bool coarse = mosaic.cell > 200;
    for (int z = std::max(reach, apart); z < side - std::max(reach, apart); z += stride) {
        for (int x = std::max(reach, apart); x < side - std::max(reach, apart); x += stride) {
            if (Material(mosaic.rock.at(x, z)) != Material::Sandstone || !mosaic.dry(x, z)) continue;
            float low = 1e30f, high = -1e30f;
            for (int dz = -reach; dz <= reach; ++dz)
                for (int dx = -reach; dx <= reach; ++dx) low = std::min(low, mosaic.top(x + dx, z + dz)), high = std::max(high, mosaic.top(x + dx, z + dz));
            const double relief = high - low;
            if (relief < (coarse ? 0.5 : 1.0) * CityReliefLow || relief > (coarse ? 2.0 : 1.0) * CityReliefHigh) continue;
            ScoutSite site;
            site.at = cellCentre(mosaic, x, z);
            site.ground = mosaic.ground.at(x, z);
            site.level = mosaic.level;
            if (coarse) {
                site.score = -mosaic.cover.at(x, z);
                site.reason = formatText("sandstone at the surface of a region cell with %.0f m of relief within 1 km, %.1f m of cover", relief, double(mosaic.cover.at(x, z)));
            } else {
                const auto under = [&](int cx, int cz) { return sandstoneUnder(planet, cellCentre(mosaic, cx, cz), mosaic.datum.at(cx, cz), mosaic.ground.at(cx, cz) - mosaic.cover.at(cx, cz)); };
                const SandstoneRun here = under(x, z);
                if (here.thickness < CityThickness) continue;
                // Dip from the height of the run's base a hundred metres to the east and to the south.
                const SandstoneRun east = under(x + apart, z), south = under(x, z + apart);
                const double dip = east.thickness > 0 && south.thickness > 0 ? std::hypot(east.base - here.base, south.base - here.base) / (apart * mosaic.cell) : 1.0;
                if (dip > CityDip) continue;
                site.score = here.thickness / CityThickness - dip / CityDip - mosaic.cover.at(x, z) / 5.0;
                site.reason = formatText("sandstone %.0f m thick from %.0f m below the rock surface, beds dipping %.1f deg, %.0f m of relief within 1 km, %.1f m of cover",
                                         double(here.thickness), double(here.starts), std::atan(dip) * DegreesPerRadian, relief, double(mosaic.cover.at(x, z)));
            }
            const Vec2 down = normalized({double(mosaic.ground.at(x - 1, z) - mosaic.ground.at(x + 1, z)), double(mosaic.ground.at(x, z - 1) - mosaic.ground.at(x, z + 1))});
            site.stand = site.at + down * std::max(200.0, 3 * mosaic.cell);
            site.lookUp = 10;
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, std::max(3000.0, 3 * mosaic.cell));
}

} // namespace derivean::sim
