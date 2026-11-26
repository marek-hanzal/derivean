#include "volcanic/fn/Source.hpp"

#include "core/fn/Hash.hpp"
#include "planet/fn/Heat.hpp"
#include "strata/type/Column.hpp"

namespace derivean::sim {
namespace {

// Points of a planet cell at which the nearest centre is asked for. No two centres lie closer
// together than seven kilometres, so every centre inside the cell is the nearest one to some point.
constexpr int Points = 16;

} // namespace

std::vector<std::vector<Centre>> centresIn(const Planet& planet, int cellX, int cellZ, const std::vector<double>& times) {
    const size_t epochs = times.size();
    std::vector<std::vector<Centre>> centres(epochs);
    std::vector<std::vector<std::pair<int64_t, int64_t>>> lattice(epochs);
    std::vector<std::vector<double>> off(epochs);
    const double pitch = PlanetCell / Points;
    for (int z = 0; z < Points; ++z) {
        for (int x = 0; x < Points; ++x) {
            const Vec2 point{cellX * PlanetCell + (x + 0.5) * pitch, cellZ * PlanetCell + (z + 0.5) * pitch};
            const Vec2 material = crustFrame(planet, point);
            for (size_t e = 0; e < epochs; ++e) {
                const MagmaticCentre found = centreNear(planet, point, material, times[e]);
                if (std::floor(found.position.x / PlanetCell) != cellX || std::floor(found.position.z / PlanetCell) != cellZ) continue;
                // The lattice steps where crust of different travel meets. A site reckoned from one
                // side of such a step may fall on ground of the other side, where it is nobody's
                // centre: a centre is one only if the ground it stands on names it too.
                const MagmaticCentre there = centreNear(planet, found.position, times[e]);
                if (there.cellX != found.cellX || there.cellZ != found.cellZ) continue;
                const double distance = length(found.position - point);
                size_t known = 0;
                while (known < centres[e].size() && lattice[e][known] != std::pair{found.cellX, found.cellZ}) ++known;
                if (known == centres[e].size()) {
                    centres[e].push_back({});
                    lattice[e].push_back({found.cellX, found.cellZ});
                    off[e].push_back(1e30);
                }
                // Where plates meet, the place of a centre is known best from the point nearest to it.
                if (distance >= off[e][known]) continue;
                off[e][known] = distance;
                centres[e][known] = {found.position, found.vigor, hash(hash(planet.seed, found.cellX, found.cellZ, 0x56454e54), found.epoch), found.epoch};
            }
        }
    }
    return centres;
}

std::vector<Centre> centresIn(const Planet& planet, int cellX, int cellZ, double time) { return centresIn(planet, cellX, cellZ, std::vector<double>{time}).front(); }

} // namespace derivean::sim
