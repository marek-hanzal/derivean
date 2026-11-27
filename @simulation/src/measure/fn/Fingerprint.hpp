#pragma once

#include "landscape/type/Terrain.hpp"

#include <cstdint>

namespace derivean::sim {

// One hash over every field a finished tile keeps: its ground, water, flow, ice, detail, shore,
// volcanic, sheet and wind records and its ledger; of a vent's eruptions and episodes only how many
// it keeps. Two builds that give a tile the same print made
// it to the bit. A field added to Tile or to one of its records goes in here too.
uint64_t fingerprintOf(const Tile& tile);

} // namespace derivean::sim
