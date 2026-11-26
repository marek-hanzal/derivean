#include "groundwater/fn/Refine.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

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
            for (size_t index = 0; index < tile->springs.size(); ++index) {
                if (!ventNear(*tile, index, low, high)) continue;
                const Spring spring = refineVent(world, *tile, index);
                if (spring.position.x >= low.x && spring.position.z >= low.z && spring.position.x < high.x && spring.position.z < high.z) springs.push_back(spring);
            }
        }
    return springs;
}

} // namespace derivean::sim
