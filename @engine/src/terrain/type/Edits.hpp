#pragma once

#include "world/type/VoxelWorld.hpp"

#include <span>

namespace derivean {

inline constexpr int ChunkVolume = ChunkSide * ChunkSide * ChunkSide;
inline constexpr uint8_t Untouched = 0xff;

// Player changes to one chunk: per voxel Untouched or the material now there, indexed (y, z, x)
// like generated voxels. Applied on top of the untouched world every time the chunk is loaded.
using ChunkEdits = std::array<uint8_t, ChunkVolume>;

// Material that changed hands in one action, summed per pair of materials. This is the record
// the simulation will take over to learn what players did to its world.
struct Edit {
    Position position;      // m, centroid of the changed voxels
    double time{};          // s of play
    double volume{};        // m³
    Material before, after;
};

struct EditStore {
    std::unordered_map<Cell, std::unique_ptr<ChunkEdits>, CellHash> chunks;
    std::unordered_map<Cell, std::vector<int>, CellHash> levels;   // column {x, 0, z}: chunk levels with edits
    std::vector<Edit> log;
};

void recordEdits(EditStore& store, std::span<const VoxelChange> changes, double time);
std::vector<Edit> takeEdits(EditStore& store);

} // namespace derivean
