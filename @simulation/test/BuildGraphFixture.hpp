#pragma once

#include "simulation/type/World.hpp"

#include <memory>

namespace derivean::sim {

// Puts finished tiles into a fresh world, so a test runs on prepared inputs without building the
// planet's terrain. The world's atlas and aquifers name this struct their friend.
struct BuildGraphFixture {
    static void seed(World& world, std::shared_ptr<const Tile> tile) {
        const TileKey key = tile->key;
        world.atlas.slots_.emplace(key, Atlas::Slot{std::move(tile)});
        ++world.atlas.finished_;
    }
    static void seed(World& world, std::shared_ptr<const GroundwaterTile> aquifer) {
        const TileKey key = aquifer->key;
        world.groundwater.slots_.emplace(key, Aquifers::Slot<GroundwaterTile>{std::move(aquifer)});
    }
    static void seed(World& world, std::shared_ptr<const Tile> site, std::shared_ptr<const GroundwaterTile> aquifer) {
        seed(world, std::move(site));
        seed(world, std::move(aquifer));
    }
};

} // namespace derivean::sim
