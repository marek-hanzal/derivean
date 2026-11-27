#pragma once

#include "landscape/type/Level.hpp"

namespace derivean::sim {

enum class BuildProduct : uint8_t { Terrain, RegionalWater, LocalWater };

struct BuildKey {
    BuildProduct product;
    TileKey tile;
    bool operator==(const BuildKey&) const = default;
};

struct BuildKeyHash {
    size_t operator()(BuildKey key) const {
        return TileKeyHash{}(key.tile) * 3 + size_t(key.product);
    }
};

} // namespace derivean::sim
