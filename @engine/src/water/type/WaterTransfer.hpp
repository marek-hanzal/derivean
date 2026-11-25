#pragma once

#include "water/type/Water.hpp"

namespace derivean {

void addWater(WaterState& state, Cell point, int delta);
void tickWater(WaterState& state, const VoxelWorld& world);
void applyBackwater(WaterState& state, const VoxelWorld& world, std::span<const Cell> active);
void moveWaterParcels(WaterState& state, const VoxelWorld& world, double step,
                      const JoltRuntime* physics, double lowestMoving);

} // namespace derivean
