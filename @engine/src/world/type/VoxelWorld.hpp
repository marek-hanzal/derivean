#pragma once

#include "planet/constant/Scale.hpp"
#include "voxel/type/Volume.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace derivean {

enum class BrushShape { Sphere, Cube };

struct TerrainTile {
    static constexpr int CellCount = VoxelsPerMeter * VoxelsPerMeter * VoxelsPerMeter;
    using Detail = std::array<Material, CellCount>;

    Material uniform{Material::Air};
    std::unique_ptr<Detail> detail;
    int occupied{};
};

// What rock that has no voxels yet answers as: solid, so nothing flows, falls or collapses into it.
inline constexpr Material Unloaded = Material::Bedrock;

struct VoxelChange {
    Cell point;
    Material before, after;
};

// One generated chunk on its way into a streamed world. Only occupied tiles are listed.
struct ChunkVoxels {
    Cell key;
    std::vector<std::pair<Cell, TerrainTile>> tiles;
    size_t occupied{};
    uint8_t openFaces{};   // bit per Neighbors entry: that border layer holds air
};

// A world that continues beyond its voxels. Chunks outside `loaded` exist but are not materialised;
// they read as Unloaded unless the column's `sky` says they are open air.
struct StreamedExtent {
    std::unordered_set<Cell, CellHash> loaded;       // chunks whose voxels are authoritative, empty ones included
    std::unordered_map<Cell, int, CellHash> sky;     // column {x, 0, z}: chunk level from which unloaded chunks are air
    std::vector<VoxelChange> journal;                // every setVoxel since the owner last drained it
};

inline constexpr int WorldFloorLevel = int(sim::WorldFloor / (ChunkSide * double(VoxelSize)));

struct VoxelWorld {
    std::unordered_map<Cell, TerrainTile, CellHash> tiles;
    std::unordered_map<Cell, size_t, CellHash> chunks;
    int voidLevel{WorldFloorLevel};                  // chunk level below which there is nothing
    std::optional<StreamedExtent> streamed;          // absent: the stored voxels are all there is
};

Material materialAt(const VoxelWorld& world, Cell point);
Voxels sampleWorld(const VoxelWorld& world, Bounds bounds);
void setVoxel(VoxelWorld& world, Cell point, Material material);
void fillBox(VoxelWorld& world, Bounds area, Material material);
bool removeVoxel(VoxelWorld& world, Cell point);
std::vector<Cell> carve(VoxelWorld& world, Cell center, int radius, BrushShape shape, double* excavatedKg = nullptr);

// The one definition of matter leaving the world: its highest point, in metres, lies under the
// world floor. Every escape ledger asks here. Space that is merely not loaded is never the void:
// it reads as solid, so whatever reaches it waits at its edge.
bool inVoid(const VoxelWorld& world, double highestPoint);

// True where voxels are authoritative: everywhere without a streamed extent, else in loaded chunks,
// above a column's sky and in the void.
bool known(const VoxelWorld& world, Cell chunk);
void loadChunk(VoxelWorld& world, ChunkVoxels&& chunk);
void unloadChunk(VoxelWorld& world, Cell chunk);

} // namespace derivean
