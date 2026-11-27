#pragma once

#include <vector>

namespace derivean::sim {

// What the circulation's closures add to the barotropic step besides advection (S4 plan, Codex
// DREV-CLAUDE-007-01 point 1), per cell; an empty vector is no such term. `tendency` (1/s²) joins
// the advection's −∇·(v (ζ + f)): relaxation, eddy growth, damping. `friction` is a rate r (1/s,
// finite and ≥ 0) that the step applies implicitly as keep = 1 / (1 + span · r) over its own span,
// 2Δt in the leapfrog and Δt in the forward step after a restart, so the caller need not know which
// step comes. The step keeps ζ's area mean at zero (docs/handoff/atmosphere-sphere.md, forcing).
struct SphereForcing {
    std::vector<double> tendency;
    std::vector<double> friction;
};

} // namespace derivean::sim
