#pragma once

#include "atmosphere/type/SpherePoisson.hpp"
#include "atmosphere/type/SphereStencil.hpp"

#include <vector>

namespace derivean::sim {

// With `multigrid`, CG is preconditioned by a V-cycle over the coarser levels; without, by the
// diagonal (the two are compared in docs/handoff/atmosphere-sphere.md). With `narrow` the V-cycle
// runs in float32 above the coarsest level; CG, its residual and the result stay double.
SpherePoisson makeSpherePoisson(const SphereGrid& grid, const SphereStencil& stencil, bool multigrid = true, bool narrow = true);

// Solves Δ result = source for the result with zero area mean. The source's area mean is the part no
// field on a closed sphere can have as a Laplacian; it is removed and reported. `result` on entry is
// the first guess (the last step's stream function makes a good one) and must have one value per
// cell. `product`, when given, is K · result on entry to rounding, used for the first residual from a
// tolerance of 1e-9 up (SpherePoisson.cpp: GivenStart); below it, and without it, the solver forms
// K · result itself. Stops when ‖K ψ − b‖ ≤ tolerance · ‖b‖ or after `limit` iterations. Below a
// tolerance of 1e-10 the report comes from K of the result as returned, left in poisson.product
// (PoissonReport::product).
PoissonReport solveSpherePoisson(SpherePoisson& poisson, const SphereGrid& grid, const std::vector<double>& source,
                                 std::vector<double>& result, double tolerance, int limit,
                                 const std::vector<double>* product = nullptr);

} // namespace derivean::sim
