#pragma once

#include "landscape/type/Level.hpp"

#include <array>
#include <vector>

namespace derivean::sim {

// A solve reads its own landscape level in row-major order, with its tile in the middle.
std::array<TileKey, 9> aquiferTerrain(TileKey key);

// A Local solve also reads the regional water below its field, in corner order.
std::vector<TileKey> aquiferRegions(TileKey local);

} // namespace derivean::sim
