#pragma once

#include "atmosphere/type/SpherePoisson.hpp"

#include <vector>

namespace derivean::sim {

// The coarser levels below `finest` down to level 1 (24 cells), each with the Laplacian rediscretised
// by makeLaplacian on its own grid.
std::vector<PoissonLevel> coarserLevels(int finest);

// One V-cycle for K z = r on levels[0]: symmetric Gauss–Seidel smoothing (forward before, backward
// after the coarse correction), restriction by summing the four children (K is an integrated
// operator, so residuals add), prolongation by the parent's value, and conjugate gradients to rounding
// on the coarsest level. With the restriction the transpose of the prolongation and the sweeps
// mirrored, the cycle is a symmetric operator, as preconditioned CG requires.
void vCycle(std::vector<PoissonLevel>& levels, int smoothing, const std::vector<double>& r, std::vector<double>& z);

} // namespace derivean::sim
