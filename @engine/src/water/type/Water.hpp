#pragma once

#include "particles/type/Particles.hpp"
#include "voxel/type/Mesh.hpp"
#include "world/type/VoxelWorld.hpp"

#include <unordered_set>

namespace derivean {

class JoltRuntime;

inline constexpr int WaterUnits = 256;
inline constexpr double WaterTick = 1.0 / 30.0;

struct WaterParcel {
    Position position;
    Position velocity;
    int amount;
};

struct WaterFoam {
    Position position;
    double life;
};

struct WaterBudget {
    uint64_t initial{}, bulk{}, airborne{}, escaped{};
};

struct WaterState {
    std::unordered_map<Cell, uint16_t, CellHash> cells;
    std::unordered_map<Cell, size_t, CellHash> chunks;
    std::unordered_set<Cell, CellHash> awake;
    std::unordered_set<Cell, CellHash> meshDirty;
    std::vector<WaterParcel> parcels;
    std::vector<WaterFoam> foam;
    WaterBudget budget;
    double accumulator{};
    uint64_t ticks{};
};

int waterAt(const WaterState& state, Cell point);
void seedWater(WaterState& state, const VoxelWorld& world, Bounds area, int fill = WaterUnits);
void wakeWater(WaterState& state, Cell point);
void wakeWater(WaterState& state, std::span<const Cell> changed);
bool displaceWater(WaterState& state, const VoxelWorld& world, Cell point);
void advanceWater(WaterState& state, const VoxelWorld& world, double step,
                  const JoltRuntime* physics = nullptr, double lowestMoving = 1e100);
FaceMesh meshWater(const WaterState& state, const VoxelWorld& world, Cell chunk);
void projectWaterParticles(ParticleProjection& output, const WaterState& state, Bounds visible);
std::optional<float> waterDepth(const WaterState& state, Position point);

} // namespace derivean
