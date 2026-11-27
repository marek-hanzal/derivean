#pragma once

#include "atmosphere/type/StencilMatrix.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// One level of the multigrid hierarchy: the same Laplacian rediscretised on a coarser level of the
// face quadtree, with its work space. Cell c of level l has the children (face, 2i + di, 2j + dj) on
// level l + 1, so restriction and prolongation need no tables.
struct PoissonLevel {
    int level{};
    int side{};
    StencilMatrix stiffness;
    std::vector<double> inverseDiagonal;
    double largest{};                                          // λmax of D⁻¹K, estimated
    std::vector<double> solution, source, residual, direction, product;   // work space
};

// Inverting the weather grid's Laplacian, Δψ = ζ, by conjugate gradients (Hestenes & Stiefel 1952) on
// the subspace without the constant, which is the stiffness matrix's only null mode, preconditioned by
// one multigrid V-cycle (Brandt 1977) over the coarser quadtree levels or, without them, by the
// diagonal.
struct SpherePoisson {
    std::vector<PoissonLevel> levels;             // [0] is the grid's own level, then ever coarser
    bool multigrid{};                             // else the preconditioner is levels[0]'s diagonal
    int smoothing{3};                             // Chebyshev degree before and after the coarse step, ≥ 1
    // Work space, no state between solves; one solve at a time per SpherePoisson.
    std::vector<double> target, residual, direction, product, preconditioned;
};

// What a solve did. A solve that misses its tolerance says so here and leaves its best iterate in
// the result; it is never silently replaced.
struct PoissonReport {
    int iterations{};
    double residual{};        // ‖K ψ − b‖ / ‖b‖ at the end, from a fresh product
    double sourceMean{};      // area mean of the source, removed before solving (zero for a vorticity)
    bool converged{};
};

} // namespace derivean::sim
