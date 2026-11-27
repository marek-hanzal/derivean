#pragma once

#include "atmosphere/type/SphereClosures.hpp"
#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SphereForcing.hpp"
#include "atmosphere/type/SphereHour.hpp"
#include "atmosphere/type/SpherePoisson.hpp"
#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// The closures over `bands` bands of latitude, the damping made from `poisson`'s own K, the friction
// from `height` (m above sea level per cell, at least zero; empty for sea level everywhere); `jet`
// starts at zero.
SphereClosures makeSphereClosures(const SphereGrid& grid, const SpherePoisson& poisson, const std::vector<double>& height, int bands);

// Σ_b B_b jet_b, rounding apart, is zero: one finite value per band and |Σ_b B_b jet_b| ≤
// JetBalance · Σ_b B_b |jet_b|. A jet from zonalWindVorticity telescopes so; the step takes ζ's area
// mean out whatever the forcing adds, so a jet that did not would fight that projection unseen.
inline constexpr double JetBalance = 1e-12;
[[nodiscard]] bool balancedJet(const SphereClosures& closures);

// Whether the closures may force a flow that inverts with `poisson`: their damping was made from its
// K and their jet is balancedJet.
[[nodiscard]] bool closuresFit(const SphereClosures& closures, const SpherePoisson& poisson);

// The forcing of the next step from the level it starts from (baseLevel): per band
// (jet − ⟨level⟩) / relaxation spread back to the cells, plus the damping with `poisson`'s K; the
// friction rates. Throws std::invalid_argument unless closuresFit, before `forcing` changes.
void fillClosures(SphereClosures& closures, const SphereGrid& grid, const SpherePoisson& poisson, const SphereFlow& flow,
                  SphereForcing& forcing);

// One window of the flow forced by the closures, its steps held to the same damping's bound: sets
// hour.stiffest to the damping's stiffness, then advanceSphereHour with fillClosures' tendency as the
// source and the closures' friction fixed for the window, read in place; the same forcing as
// fillClosures', bit for bit. Throws std::invalid_argument unless closuresFit, before anything
// changes, hour.stiffest included; checked once a window.
// Once the window has begun, hour.stiffest stays set after its refusal or an exception from within
// it (the same call repeats the same tries).
[[nodiscard]] bool advanceClosedHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, SphereClosures& closures);

} // namespace derivean::sim
