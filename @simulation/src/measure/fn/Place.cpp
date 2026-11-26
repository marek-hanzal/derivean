#include "measure/fn/Place.hpp"

#include "landscape/fn/Forcing.hpp"
#include "measure/type/Mosaic.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {

Vec2 highestNear(const Planet& planet, Vec2 near, double reach) {
    // Landscapes read the planet through planetPoint, which shifts a position by up to a planet
    // cell; a summit is only one where it still is after that shift.
    const double step = PlanetCell / 2;
    const int span = int(reach / step);
    Vec2 found = near;
    double highest = -1e30;
    for (int dz = -span; dz <= span; ++dz)
        for (int dx = -span; dx <= span; ++dx) {
            const Vec2 here = near + Vec2{dx * step, dz * step};
            const double height = samplePlanet(planet.elevation, planetPoint(planet.seed, here));
            if (height > highest) highest = height, found = here;
        }
    return found;
}

Vec2 coastNear(World& world, Vec2 near) {
    const Mosaic mosaic = gatherMosaic(world, Level::Region, near, 3);
    const int side = mosaic.side();
    const auto sea = [&](int x, int z) { return mosaic.flooded(x, z) && !mosaic.lake(x, z); };
    Vec2 found = near;
    double best = 1e30;
    for (int z = 1; z + 1 < side; ++z) {
        for (int x = 1; x + 1 < side; ++x) {
            if (mosaic.flooded(x, z) || !(sea(x + 1, z) || sea(x - 1, z) || sea(x, z + 1) || sea(x, z - 1))) continue;
            const Vec2 centre = cellCentre(mosaic, x, z);
            const double distance = length(centre - near);
            if (distance < best) best = distance, found = centre;
        }
    }
    return found;
}

} // namespace derivean::sim
