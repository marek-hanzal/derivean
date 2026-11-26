#pragma once

#include "groundwater/type/Groundwater.hpp"
#include "groundwater/type/WaterField.hpp"

namespace derivean::sim {

// Water leaving the ground at one place.
struct VentWater {
    SpringKind kind{SpringKind::Table};
    Vec2 position;
    float elevation{};       // m
    float flow{};            // m³/yr
    Borne borne;
    float depth{};           // m below the ground of the rock it last stood in: what its heat is that of
    int32_t cell{};          // own cell of the tile
    uint8_t storey{};
    Material aquifer{Material::Air}, floor{Material::Air};
};

// Adds the spring and its record: its heat from the rock it last stood in, cooled on the way up by
// how little of it there is and capped where it boils; its silica from the hottest rock it passed,
// its carbonate from the soluble rock it crossed and the gas it met.
void addSpring(const WaterField& field, const AquiferColumn& column, const VentWater& water, GroundwaterTile& tile);

// Where the tight bed under a cell comes to daylight beside it: the mean of the points on the way
// to each neighbour at which the ground has fallen to the bed, kept inside the cell.
void lipOf(const WaterField& field, int x, int z, Vec2& position, float& elevation);

// Fields.cpp — the per-cell arrays a tile keeps and the budget of everything but its vents.
void readFields(const WaterField& field, GroundwaterTile& tile);

} // namespace derivean::sim
