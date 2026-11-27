#pragma once

#include "water/type/Water.hpp"

namespace derivean {

void addWater(WaterState& state, Cell point, int delta);
WaterBrick* brickAt(const WaterState& state, Cell tile);
void dropBrick(WaterState& state, Cell tile);
void tickWater(WaterState& state, const VoxelWorld& world);
void applyBackwater(WaterState& state, const VoxelWorld& world, std::span<const Cell> active);
void moveWaterParcels(WaterState& state, const VoxelWorld& world, double step,
                      const JoltRuntime* physics, double lowestMoving);

} // namespace derivean
