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

// One V-cycle for K z = r on levels[0]: Chebyshev–Jacobi smoothing of degree `smoothing` before and
// after the coarse correction, restriction by summing the four children (K is an integrated operator,
// so residuals add), prolongation by the parent's value, and conjugate gradients to rounding on the
// coarsest level. The smoother is a fixed polynomial p(D⁻¹K) D⁻¹, symmetric, the same before and
// after, and the restriction is the transpose of the prolongation, so the cycle is a symmetric
// operator, as preconditioned CG requires. Unlike Gauss–Seidel it has no order among the cells.
void vCycle(std::vector<PoissonLevel>& levels, int smoothing, const std::vector<double>& r, std::vector<double>& z);

} // namespace derivean::sim
