#include "groundwater/fn/Reads.hpp"

#include <algorithm>

namespace derivean::sim {

std::array<TileKey, 9> aquiferTerrain(TileKey key) {
    std::array<TileKey, 9> terrain;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx)
            terrain[size_t((dz + 1) * 3 + dx + 1)] = {key.level, key.x + dx, key.z + dz};
    return terrain;
}

std::vector<TileKey> aquiferRegions(TileKey local) {
    std::vector<TileKey> regions;
    // The field reaches less than a Local tile beyond its own tile.
    for (const int dz : {-1, 1})
        for (const int dx : {-1, 1}) {
            const TileKey region = parentOf({Level::Local, local.x + dx, local.z + dz});
            if (std::find(regions.begin(), regions.end(), region) == regions.end()) regions.push_back(region);
        }
    return regions;
}

} // namespace derivean::sim
