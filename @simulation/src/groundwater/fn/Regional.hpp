#pragma once

#include "groundwater/type/Regional.hpp"
#include "landscape/type/Terrain.hpp"

#include <memory>

namespace derivean::sim {

struct Setting;

// Region cells around a region tile its regional flow reads: 65 km.
inline constexpr int RegionalMargin = 64;
inline constexpr int RegionalCells = TileCells + 2 * RegionalMargin;

// One region cell to the regional flow.
struct RegionalColumn {
    float ground{};          // m
    float stage{};           // m, where water stands at an outlet; the ground elsewhere
    float recharge{};        // m³/yr going down into the regional flow
    float transmissivity{};  // m²/yr of the rock from the ground down to the base of circulation
    float depth{};           // m from the ground to the base of circulation
    bool confined{};         // a tight bed lies over most of what conducts: the water cannot come up here
    uint8_t outlet{};        // as RegionalTile::outlet, without 4: that one the solve finds
};

// 0..1: the share of a column's recharge that travels farther than a local tile follows and joins
// the regional flow. Closed form of the column's transmissivity (m²/yr) and of whether a tight bed
// lies over its conducting rock: water under such a bed cannot come up into the streams beside it.
// Set after the share local tiles leave under way (groundwater.md, section 5: granite hills a few
// per cent, ordinary country a tenth to a third, faulted or confined rock most of it); not yet
// fitted against Local runs.
float deepShareOf(float transmissivity, bool confined);

// The columns of the region tile `key` and RegionalMargin cells around it, row-major over
// RegionalCells², from the nine finished region tiles around it (row-major, the tile in the middle).
void readRegionalColumns(const Setting& setting, TileKey key, const Tile* const around[9], std::vector<RegionalColumn>& field);

// The regional flow over such a field. The water stands as low as its outlets let it: a least
// gradient by recharge over transmissivity rises from them, and where that would lift it above the
// ground the table meets the ground and the water comes up there, except where a tight bed holds
// it down. Each cell's water follows the way its head was set by to where that way begins.
std::shared_ptr<RegionalTile> solveRegional(TileKey key, const std::vector<RegionalColumn>& field);

std::shared_ptr<RegionalTile> buildRegional(const Setting& setting, TileKey key, const Tile* const around[9]);

} // namespace derivean::sim
