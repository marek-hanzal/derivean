#pragma once

#include "voxel/type/Volume.hpp"

namespace derivean {

// Eight shorts per quad: axis, sign, plane, u, v, width, height, material.
// Coordinates are in voxels relative to the projection's origin.
using FaceMesh = std::vector<int16_t>;

FaceMesh meshFaces(const Voxels& voxels, Bounds inner);
std::vector<Bounds> mergeSolids(Voxels voxels);

} // namespace derivean
