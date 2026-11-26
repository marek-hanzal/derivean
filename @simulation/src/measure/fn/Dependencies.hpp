#pragma once

#include "landscape/fn/Build.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

// The finished tiles one tile waits for, kept alive for as long as a replay by hand reads them.
struct Dependencies {
    std::vector<TileKey> keys;
    std::vector<std::shared_ptr<const Tile>> tiles;

    // Answers exactly these keys, as the atlas does for the tile it builds.
    TileLookup lookup() const {
        return [keys = keys, tiles = tiles](TileKey wanted) -> const Tile* {
            for (size_t i = 0; i < keys.size(); ++i)
                if (keys[i] == wanted) return tiles[i].get();
            return nullptr;
        };
    }
};

// Builds whatever of them is missing.
inline Dependencies requireDependencies(World& world, TileKey key) {
    Dependencies needed{dependencies(key), {}};
    for (const TileKey& other : needed.keys) needed.tiles.push_back(world.atlas.require(other));
    return needed;
}

} // namespace derivean::sim
