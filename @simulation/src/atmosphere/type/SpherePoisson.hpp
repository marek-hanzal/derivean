#pragma once

#include "atmosphere/type/StencilMatrix.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// One level of the multigrid hierarchy: the same Laplacian rediscretised on a coarser level of the
// face quadtree, with its work space. Cell c of level l has the children (face, 2i + di, 2j + dj) on
// level l + 1, so restriction and prolongation need no tables. The float32 V-cycle's levels are the
// double ones rounded (narrowLevels).
template <class Real> struct PoissonLevelOf {
    int level{};
    int side{};
    StencilMatrixOf<Real> stiffness;
    std::vector<Real> inverseDiagonal;
    double largest{};                     // λmax of D⁻¹K, estimated on smoothed levels
    std::vector<Real> solution, source;   // the coarse correction and its source (not on the finest)
    // Work space: the smoother's residual and directions, CG's on the coarsest level.
    std::vector<Real> residual, direction, spare;
};

using PoissonLevel = PoissonLevelOf<double>;

// Inverting the weather grid's Laplacian, Δψ = ζ, by conjugate gradients (Hestenes & Stiefel 1952) on
// the subspace without the constant, which is the stiffness matrix's only null mode, preconditioned by
// one multigrid V-cycle (Brandt 1977) over the coarser quadtree levels or, without them, by the
// diagonal.
struct SpherePoisson {
    std::vector<PoissonLevel> levels;             // [0] is the grid's own level, then ever coarser;
                                                  // with `narrow`, only [0] (no work space) and the coarsest
    bool multigrid{};                             // else the preconditioner is levels[0]'s diagonal
    int smoothing{3};                             // Chebyshev degree before and after the coarse step, ≥ 1
    // The V-cycle in float32, a low-precision preconditioner inside the double CG (Higham & Mary 2022,
    // §8.2): the levels above the coarsest rounded, the coarsest solved in double; the last entry holds
    // only the coarsest's float source and correction. Empty when the V-cycle runs in double.
    std::vector<PoissonLevelOf<float>> narrow;
    // Work space, no state between solves; one solve at a time per SpherePoisson.
    std::vector<double> target, residual, direction, product, preconditioned;
};

// What a solve did. A solve that misses its tolerance says so here and leaves its best iterate in
// the result; it is never silently replaced.
struct PoissonReport {
    int iterations{};
    double residual{};        // ‖K ψ − b‖ / ‖b‖ at the end: below a tolerance of 1e-10 from a fresh
                              // product of ψ as returned, from 1e-10 up CG's recursive estimate (SpherePoisson.cpp:
                              // TrustedResidual, GivenStart)
    double sourceMean{};      // area mean of the source, removed before solving (zero for a vorticity)
    bool converged{};
    bool product{};           // the solver's `product` holds K · result, formed for the residual below 1e-10
};

} // namespace derivean::sim
