#pragma once

#include "atmosphere/type/SpherePoisson.hpp"

#include <vector>

namespace derivean::sim {

// One multigrid V-cycle (Brandt 1977) for K z = r on levels[0]: Chebyshev–Jacobi smoothing of degree
// `smoothing` before and after the coarse correction, restriction by summing the four children (K is
// an integrated operator, so residuals add), prolongation by the parent's value, and conjugate gradients to rounding on the
// coarsest level. The smoother is a fixed polynomial p(D⁻¹K) D⁻¹, symmetric, the same before and
// after, and the restriction is the transpose of the prolongation, so the cycle is a symmetric
// operator, as preconditioned CG requires. Unlike Gauss–Seidel it has no order among the cells.
void vCycle(std::vector<PoissonLevel>& levels, int smoothing, const std::vector<double>& r, std::vector<double>& z);

// The same V-cycle in float32 on `narrow`, the coarsest level solved in double on `coarsest`; r and z
// stay double. It is the double cycle up to float32 rounding (under 6e-7 of z's largest value on
// levels 4 to 8), so its symmetry holds to that rounding too.
void vCycle(std::vector<PoissonLevelOf<float>>& narrow, PoissonLevel& coarsest, int smoothing, const std::vector<double>& r,
            std::vector<double>& z);

} // namespace derivean::sim
