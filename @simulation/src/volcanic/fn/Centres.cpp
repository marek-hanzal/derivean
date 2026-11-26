#include "volcanic/fn/Source.hpp"

#include "core/fn/Hash.hpp"
#include "planet/fn/Heat.hpp"

namespace derivean::sim {
namespace {

// Points of a planet cell at which the nearest centre is asked for. No two centres lie closer
// together than seven kilometres, so every centre inside the cell is the nearest one to some point.
constexpr int Points = 16;

} // namespace

std::vector<Centre> centresIn(const Planet& planet, int cellX, int cellZ) {
    std::vector<Centre> centres;
    std::vector<std::pair<int64_t, int64_t>> lattice;
    std::vector<double> off;
    const double pitch = PlanetCell / Points;
    for (int z = 0; z < Points; ++z) {
        for (int x = 0; x < Points; ++x) {
            const Vec2 point{cellX * PlanetCell + (x + 0.5) * pitch, cellZ * PlanetCell + (z + 0.5) * pitch};
            const MagmaticCentre found = centreNear(planet, point, planet.time);
            if (std::floor(found.position.x / PlanetCell) != cellX || std::floor(found.position.z / PlanetCell) != cellZ) continue;
            // The lattice steps where crust of different travel meets. A site reckoned from one side
            // of such a step may fall on ground of the other side, where it is nobody's centre: a
            // centre is one only if the ground it stands on names it too.
            const MagmaticCentre there = centreNear(planet, found.position, planet.time);
            if (there.cellX != found.cellX || there.cellZ != found.cellZ) continue;
            const double distance = length(found.position - point);
            size_t known = 0;
            while (known < centres.size() && lattice[known] != std::pair{found.cellX, found.cellZ}) ++known;
            if (known == centres.size()) {
                centres.push_back({});
                lattice.push_back({found.cellX, found.cellZ});
                off.push_back(1e30);
            }
            // Where plates meet, the place of a centre is known best from the point nearest to it.
            if (distance >= off[known]) continue;
            off[known] = distance;
            centres[known] = {found.position, found.vigor, hash(planet.seed, found.cellX, found.cellZ, 0x56454e54)};
        }
    }
    return centres;
}

} // namespace derivean::sim
