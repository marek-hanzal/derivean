#include "measure/fn/Scout.hpp"

#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Format.hpp"
#include "planet/fn/Heat.hpp"

namespace derivean::sim {
namespace {

constexpr float HotGradient = 0.15f;   // K/m near the surface
constexpr float WetEnough = 0.3f;      // m/yr of rain that keeps ground water moving through hot rock

} // namespace

std::vector<ScoutSite> findHotGround(const Mosaic& mosaic, const Planet& planet, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side(), stride = cellsIn(mosaic, 1500);
    for (int z = stride; z < side - stride; z += stride) {
        for (int x = stride; x < side - stride; x += stride) {
            if (!mosaic.dry(x, z) || mosaic.rain.at(x, z) < WetEnough) continue;
            const Vec2 at = cellCentre(mosaic, x, z);
            const Geotherm heat = geothermAt(planet, planetPoint(planet.seed, at));
            if (heat.gradient <= HotGradient) continue;
            ScoutSite site;
            site.at = at;
            site.ground = mosaic.ground.at(x, z);
            site.level = mosaic.level;
            site.score = heat.gradient;
            site.reason = formatText("hot ground: %.0f K/km from below, %.0f %% of it from melt that rose %.2f Myr ago, closeness to its centre %.2f; rain %.2f m/yr, "
                                     "rock at 100 m depth %.0f C",
                                     1000.0 * heat.gradient, 100.0 * heat.magmatic, heat.sinceMelt / 1e6, double(heat.centre), double(mosaic.rain.at(x, z)),
                                     double(rockTemperature(heat, mosaic.warmth.at(x, z), 100.0f)));
            // Seen from lower ground nearby.
            const Vec2 down = normalized({double(mosaic.ground.at(x - 1, z) - mosaic.ground.at(x + 1, z)), double(mosaic.ground.at(x, z - 1) - mosaic.ground.at(x, z + 1))});
            site.stand = site.at + down * std::max(400.0, 3 * mosaic.cell);
            site.lookUp = 5;
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, 10000);
}

} // namespace derivean::sim
