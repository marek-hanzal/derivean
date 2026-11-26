#include "groundwater/fn/Query.hpp"

#include "ground/fn/SpringBounds.hpp"
#include "groundwater/fn/Refine.hpp"
#include "simulation/type/World.hpp"

#include <stdexcept>
#include <string>

namespace derivean::sim {
namespace {

template <class Refine>
void collectSprings(const GroundwaterTile& tile, Vec2 low, Vec2 high,
                    std::vector<Spring>& springs, Refine&& refine) {
    for (size_t index = 0; index < tile.springs.size(); ++index) {
        if (!ventNear(tile, index, low, high)) continue;
        const Spring spring = refine(tile, index);
        if (spring.position.x >= low.x && spring.position.z >= low.z &&
            spring.position.x < high.x && spring.position.z < high.z) springs.push_back(spring);
    }
}

} // namespace

std::shared_ptr<const GroundwaterTile> groundwaterAt(World& world, TileKey local) { return world.groundwater.require(world.atlas, local); }

std::shared_ptr<const CaveField> caveFieldAt(World& world, Vec2 position) { return groundwaterAt(world, tileAt(Level::Local, position))->caves; }

std::vector<Spring> springsIn(World& world, Vec2 low, Vec2 high) {
    std::vector<Spring> springs;
    // A vent lies in the local cell its water left the ground in, so only springs of cells that
    // reach into the rectangle are put on the finer ground and asked where they are.
    const TileKey first = tileAt(Level::Local, low), last = tileAt(Level::Local, high);
    for (int z = first.z; z <= last.z; ++z)
        for (int x = first.x; x <= last.x; ++x) {
            const std::shared_ptr<const GroundwaterTile> tile = groundwaterAt(world, {Level::Local, x, z});
            collectSprings(*tile, low, high, springs, [&](const GroundwaterTile& aquifer, size_t index) {
                return refineVent(world, aquifer, index);
            });
        }
    return springs;
}

std::vector<Spring> groundSprings(TileKey ground, const TileLookup& terrain, const AquiferLookup& aquifers) {
    const SpringBounds bounds = groundSpringBounds(ground);
    std::vector<Spring> springs;
    for (const TileKey local : groundwaterRead(ground)) {
        const auto aquifer = aquifers(local);
        if (!aquifer) throw std::logic_error("missing prepared Local aquifer for ground springs");
        collectSprings(*aquifer, bounds.low, bounds.high, springs, [&](const GroundwaterTile& tile, size_t index) {
            const TileKey key = ventSite(tile, index);
            const Tile* site = terrain(key);
            if (!site) throw std::logic_error("undeclared Site input for ground spring at tile (" +
                                              std::to_string(key.x) + ", " + std::to_string(key.z) + ")");
            return refineVent(*site, tile, index);
        });
    }
    return springs;
}

} // namespace derivean::sim
