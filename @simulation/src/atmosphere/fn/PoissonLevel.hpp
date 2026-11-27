#pragma once

#include "atmosphere/type/SpherePoisson.hpp"
#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// What a level is built for: the finest level of a V-cycle is smoothed but its source and correction
// are the caller's; a coarser level is smoothed and holds both; the coarsest is solved by CG; a level
// without multigrid only lends its diagonal.
enum class LevelUse { Finest, Coarser, Coarsest, Diagonal };

// One level: its Laplacian with the faces' interiors shared (shareFaceInteriors), the inverse
// diagonal and, on smoothed levels, the largest eigenvalue of D⁻¹K, which bounds the smoother's
// interval.
PoissonLevel makePoissonLevel(const SphereGrid& grid, const SphereStencil& stencil, LevelUse use);

// The coarser levels below `finest` down to level 1 (24 cells), each with the Laplacian rediscretised
// by makeLaplacian on its own grid.
std::vector<PoissonLevel> coarserLevels(int finest);

// The levels above the coarsest rounded to float32 for the mixed-precision V-cycle (Higham & Mary
// 2022, §8.2), with the coarsest's float source and correction as the last entry.
std::vector<PoissonLevelOf<float>> narrowLevels(const std::vector<PoissonLevel>& levels);

} // namespace derivean::sim
