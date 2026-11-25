#pragma once

#include "voxel/type/Volume.hpp"

#include <map>
#include <memory>
#include <unordered_map>

namespace derivean {

enum class BrushShape { Sphere, Cube };

struct TerrainTile {
    static constexpr int CellCount = VoxelsPerMeter * VoxelsPerMeter * VoxelsPerMeter;
    using Detail = std::array<Material, CellCount>;

    Material uniform{Material::Air};
    std::unique_ptr<Detail> detail;
    int occupied{};
};

struct VoxelWorld {
    std::unordered_map<Cell, TerrainTile, CellHash> tiles;
    std::unordered_map<Cell, size_t, CellHash> chunks;
    std::map<int, size_t> occupiedLevels;
};

Material materialAt(const VoxelWorld& world, Cell point);
Voxels sampleWorld(const VoxelWorld& world, Bounds bounds);
void setVoxel(VoxelWorld& world, Cell point, Material material);
void fillBox(VoxelWorld& world, Bounds area, Material material);
bool removeVoxel(VoxelWorld& world, Cell point);
std::vector<Cell> carve(VoxelWorld& world, Cell center, int radius, BrushShape shape, double* excavatedKg = nullptr);
bool belowTerrain(const VoxelWorld& world, double highestPoint);

} // namespace derivean
