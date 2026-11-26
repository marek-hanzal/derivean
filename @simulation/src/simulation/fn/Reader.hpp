#pragma once

#include "simulation/fn/Query.hpp"

#include <array>

namespace derivean::sim {

// Reads finished tiles of one level by global cell index. It keeps the last few tiles it touched,
// so a run of nearby samples takes the atlas lock only when it crosses into a new tile.
struct TileReader {
    const Atlas& atlas;
    Level level;
    std::array<std::pair<TileKey, std::shared_ptr<const Tile>>, 4> recent{};
    int filled{}, next{};

    const Tile* tile(int32_t x, int32_t z) {
        const TileKey key{level, x, z};
        for (int i = 0; i < filled; ++i)
            if (recent[size_t(i)].first == key) return recent[size_t(i)].second.get();
        recent[size_t(next)] = {key, atlas.find(key)};
        const Tile* found = recent[size_t(next)].second.get();
        filled = std::min(filled + 1, int(recent.size()));
        next = (next + 1) % int(recent.size());
        return found;
    }
};

// Fills `sample` from the reader's level; false when the tile under the point is not finished.
bool readSurface(TileReader& reader, Vec2 position, SurfaceSample& sample);

} // namespace derivean::sim
