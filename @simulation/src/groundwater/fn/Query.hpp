#pragma once

#include "groundwater/type/Spring.hpp"
#include "landscape/fn/Build.hpp"

namespace derivean::sim {

std::vector<Spring> groundSprings(TileKey ground, const TileLookup& terrain, const AquiferLookup& aquifers);

} // namespace derivean::sim
