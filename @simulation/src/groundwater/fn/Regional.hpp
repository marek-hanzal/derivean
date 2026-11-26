#pragma once

#include "groundwater/type/Regional.hpp"
#include "groundwater/type/WaterField.hpp"
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

// 0..1: the share of a column's recharge that goes deeper and farther than the streams of its own
// hillside and joins the regional flow. Closed form of the column's transmissivity (m²/yr) and of
// whether a tight bed lies over its conducting rock: water under such a bed cannot come up into the
// streams beside it. Granite hills hand a few per cent, ordinary country a tenth to a third,
// faulted or confined rock most of it; not fitted against measured catchments.
float deepShareOf(float transmissivity, bool confined);

// What the regional flow reads of a column's beds, top first, down to the base of circulation.
struct DeepColumn {
    float transmissivity{};  // m²/yr of all of them
    float passes{1e30f};     // m/yr across the tightest bed (at least 1 m) over at least half of it: what water going deep crosses
    bool confined{};         // a tight bed (at least 5 m, under 0.05 m/yr across) lies over at least half of it
    bool karst{};            // a run of soluble rock holds conduits, at any depth: its water rises at their own springs
};
DeepColumn deepColumnOf(const WaterLayer* layers, int count);

// m³/yr of the recharge (m³/yr over `area` m²) of a column with those beds that joins the regional
// flow: its deep share, no more than its tightest bed lets down, none where conduits drain it. A
// local column hands it on (AquiferColumn::handed) and a region column takes it in: one accounting.
float deepRechargeOf(const DeepColumn& column, float recharge, float area);

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
