#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace derivean::sim {

// The water of the trunk field (routeTrunks) over its ground as it stands: where the sea is, how
// the planet's yield enters, and which way each cell drains.
struct TrunkRouting {
    std::vector<uint8_t> sea;                         // 1 below sea level
    std::vector<uint32_t> coast;                      // sea cells beside land, where the flood starts
    std::vector<uint32_t> order;                      // coast first, then each land cell after the cell it drains to
    std::vector<uint16_t> owner;                      // planet cell each cell's point maps to
    std::vector<float> yield;                         // m³/yr each land cell of a planet cell yields
    std::vector<std::pair<uint32_t, float>> drowned;  // a planet land cell with no land here: its yield, at its highest cell
};

} // namespace derivean::sim
