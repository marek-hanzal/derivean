#pragma once

#include "groundwater/type/Aquifer.hpp"
#include "groundwater/type/Groundwater.hpp"

namespace derivean::sim {

// Water leaving the ground at one place.
struct VentWater {
    SpringKind kind{SpringKind::Table};
    Vec2 position;
    float elevation{};       // m
    float flow{};            // m³/yr
    Carried carried;
    int32_t cell{};          // own cell of the tile
    uint8_t storey{};
    Material aquifer{Material::Air}, floor{Material::Air};
};

// Adds the spring and its record: its heat from the rock at the depth its water reached, cooled on
// the way up by how little of it there is, capped where it boils; its loads from that heat and
// from the rock it crossed.
void addSpring(const WaterField& field, const AquiferColumn& column, const VentWater& water, GroundwaterTile& tile);

// Where the tight bed under a cell comes to daylight beside it: the mean of the points on the way
// to each neighbour at which the ground has fallen to the bed, kept inside the cell.
void lipOf(const WaterField& field, int x, int z, Vec2& position, float& elevation);

// Fields.cpp — the per-cell arrays a tile keeps and the budget of everything but its vents.
void readFields(const WaterField& field, GroundwaterTile& tile);

} // namespace derivean::sim
