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

// The water of one tile's cells, and which of them are awake. A cell that dries keeps its wake until
// the next tick has seen it, so a brick stays while it holds water or a wake.
struct WaterBrick {
    std::array<uint16_t, TerrainTile::CellCount> amount{};
    std::array<uint64_t, TerrainTile::CellCount / 64> awake{};
    int wet{};          // cells holding water
    bool listed{};      // its key is in WaterState::rousing
};

struct WaterState {
    std::unordered_map<Cell, WaterBrick, CellHash> bricks;   // by tile
    std::unordered_map<Cell, size_t, CellHash> chunks;       // chunk: its bricks that hold water
    std::vector<Cell> rousing;                               // bricks woken since the last tick, each key once
    std::unordered_set<Cell, CellHash> meshDirty;
    // What brick lookups found; addWater and dropBrick forget the keys they change. Even a const lookup
    // writes it, so the water is read on the game thread only.
    mutable CellCache<WaterBrick> found;

    std::vector<WaterParcel> parcels;
    std::vector<WaterFoam> foam;
    WaterBudget budget;
    double accumulator{};
    uint64_t ticks{};
};

int waterAt(const WaterState& state, Cell point);
// Every cell that holds water, brick by brick.
template<typename Visit>
void forEachWater(const WaterState& state, Visit&& visit) {
    for (const auto& [tile, brick] : state.bricks) {
        if (!brick.wet) continue;
        const Cell origin = tile * VoxelsPerMeter;
        for (int i = 0; i < TerrainTile::CellCount; ++i)
            if (brick.amount[size_t(i)])
                visit(origin + Cell{i % VoxelsPerMeter, i / (VoxelsPerMeter * VoxelsPerMeter), i / VoxelsPerMeter % VoxelsPerMeter}, int(brick.amount[size_t(i)]));
    }
}
void seedWater(WaterState& state, const VoxelWorld& world, Bounds area, int fill = WaterUnits);
void wakeWater(WaterState& state, Cell point);
void wakeWater(WaterState& state, std::span<const Cell> changed);
bool displaceWater(WaterState& state, const VoxelWorld& world, Cell point);
void advanceWater(WaterState& state, const VoxelWorld& world, double step,
                  const JoltRuntime* physics = nullptr, double lowestMoving = 1e100);
FaceMesh meshWater(const WaterState& state, const VoxelWorld& world, Cell chunk);
void projectWaterParticles(ParticleProjection& output, const WaterState& state, Bounds visible, Position origin);
std::optional<float> waterDepth(const WaterState& state, Position point);

} // namespace derivean
