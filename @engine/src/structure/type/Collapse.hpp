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
    std::vector<Cell> looseSand;
};

CollapseResult collapse(VoxelWorld& world, std::vector<Cell>& changed);

} // namespace derivean
