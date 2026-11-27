#pragma once

#include "atmosphere/type/SphereFlow.hpp"

#include <functional>
#include <vector>

namespace derivean::sim {

// What the circulation's closures add to the barotropic step besides advection (S4 plan, binding
// correction 1; Codex's review DREV-CLAUDE-007-01, point 1), per cell; an empty vector is no such term.
// `tendency` (1/s²) joins the advection's −∇·(v (ζ + f)): relaxation, eddy growth, damping.
// `friction` is a rate r (1/s, finite and ≥ 0) that the step applies implicitly as
// keep = 1 / (1 + span · r) over its own span, 2Δt in the leapfrog and Δt in the forward step after
// a restart, so the caller need not know which step comes. The step keeps ζ's area mean at zero
// (docs/handoff/atmosphere-sphere.md, forcing and the hour).
struct SphereForcing {
    std::vector<double> tendency;
    std::vector<double> friction;
};

// Fills the forcing of the next wind step from the flow it steps from (its ζ, the level before, ψ),
// as closures that read the state do: relaxation, damping, eddy growth. `forcing` is the caller's
// storage, its vectors emptied before each call with their capacity kept; a term the source leaves
// empty is no term. A source may read only the flow and a context fixed for the window: a try taken
// back asks it again for the same steps, and only then is the retaken window a clean run. Time, the
// eddies and whatever else changes during the window (S4.3) go to it explicitly as the try's
// context, never as state the source keeps or reads from outside. What it throws ends the window
// with the flow and the hour as they came in (advanceSphereHour).
using SphereForcingSource = std::function<void(const SphereFlow& flow, SphereForcing& forcing)>;

} // namespace derivean::sim
