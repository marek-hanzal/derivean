#pragma once

#include <vector>

namespace derivean::sim {

// Scale-selective damping of the barotropic flow, the biharmonic hyperdiffusion of global models
// (Jablonowski & Williamson 2011; S4 plan, binding correction 3; Codex's review DREV-CLAUDE-007-01,
// point 3): the tendency −ν L² ζ of the level a step starts from, L = A⁻¹ K the Poisson's stiffness
// K over the cell areas A, standing for −Δ. For the A-weighted product ⟨ζ, −ν L² ζ⟩_A =
// −ν ‖L ζ‖²_A ≤ 0, so it only takes enstrophy out; a step of span s keeps 1 − s ν λ² of an eigenmode
// of L before the flow's filter, in [0, 1] while s ν λ_max² ≤ 1. With the filter a mode's two roots
// stay real up to s ν λ² = 0.976, the physical one positive and dominant, and the step is stable up
// to 2.06; a mode started by the forward step keeps its sign below 0.976. The check of a step holds
// s ν bound² ≤ 1 on a certified upper bound, 1.3 to 1.4 times λ_max, so s ν λ_max² ≤ 0.6 in practice.
// ν is set from an estimate of λ_max so the fastest mode decays in `decay`.
struct SphereDamping {
    double decay{};      // s, e-folding time of the fastest mode
    double estimate{};   // 1/m², λ_max of A⁻¹K by power iteration, from below
    double bound{};      // 1/m², max_i Σ_j |K_ij| / A_i ≥ λ_max (Gershgorin 1931)
    double rate{};       // ν, m⁴/s: 1 / (decay · estimate²)
    std::vector<double> work, twice;   // work space, one call at a time
};

} // namespace derivean::sim
