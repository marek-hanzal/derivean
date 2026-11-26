#pragma once

#include "world/type/VoxelWorld.hpp"

namespace derivean {

struct VoxelPiece {
    Voxels voxels;
    float mass;
    float friction;
};

struct CollapseResult {
    std::vector<VoxelPiece> pieces;
    std::vector<std::pair<Cell, Material>> loose;   // granular voxels of a fallen piece, each to become a grain of its material
};

CollapseResult collapse(VoxelWorld& world, std::vector<Cell>& changed);

} // namespace derivean
