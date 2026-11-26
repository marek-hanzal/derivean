#pragma once

#include "simulation/fn/Query.hpp"

namespace derivean {

// The material a voxel shows whose centre lies `depth` metres under the ground of `sample`: a spring's
// deposit, then loose cover in the order it lies (coarse on the rock, sand on it, fines on top)
// where the cover is thicker than that, else the sample's rock. The fines are soil near the surface
// unless they are under water or a film too thin to be one. It follows the simulation's cover rule
// as far as a surface sample can tell: ground the frost or hot water keeps bare, clay, stones and
// beds exposed inside a cell are the voxel column's to know.
sim::Material surfaceMaterial(const sim::SurfaceSample& sample, float depth);

} // namespace derivean
