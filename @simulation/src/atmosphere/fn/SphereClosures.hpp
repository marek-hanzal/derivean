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
// from `height` (m above sea level per cell, at least zero; empty for sea level everywhere), the eddy
// band of today's metres (SphereEddyWavelength; replace `band` for another, makeEddyBand), refreshed
// every SphereEddyEvery; `jet`, `growth` and the eddies start at zero.
SphereClosures makeSphereClosures(const SphereGrid& grid, const SpherePoisson& poisson, const std::vector<double>& height, int bands);

// Σ_b B_b jet_b, rounding apart, is zero: one finite value per band and |Σ_b B_b jet_b| ≤
// JetBalance · Σ_b B_b |jet_b|. A jet from zonalWindVorticity telescopes so; the step takes ζ's area
// mean out whatever the forcing adds, so a jet that did not would fight that projection unseen.
inline constexpr double JetBalance = 1e-12;
[[nodiscard]] bool balancedJet(const SphereClosures& closures);

// Whether the closures may force a flow that inverts with `poisson`: their damping was made from its
// K, their jet is balancedJet, their growth one finite rate ≥ 0 per band, their eddies one finite
// value per cell, their band made for the same grid, eddyEvery finite and positive and `since` finite
// and ≥ 0.
[[nodiscard]] bool closuresFit(const SphereClosures& closures, const SpherePoisson& poisson);

// The eddies' tendency per cell from `eddy` (one value per cell): per band the mean square of eddy
// (bandMeans), its envelope over SphereEnvelopeReach either side, triangular in bands, Σ_o (R + 1 −
// |o|) s_{b+o} / (R + 1)² with R = round(SphereEnvelopeReach / the bands' width), at least one, a band
// past a pole the band mirrored across it (where the flat world's rows wrap round); the feed growth_b ·
// max(0, 1 − envelope / SphereEddyLimit²) spread to the cells (spreadBands); times eddy. The flat
// world's growth · eddy (atmosphere/fn/Circulation.cpp). For closures that fit (closuresFit, not
// checked); `eddy` and `feeding` are neither the closures' work space nor each other.
void eddyFeeding(SphereClosures& closures, const SphereGrid& grid, const std::vector<double>& eddy, std::vector<double>& feeding);

// The forcing of the next step from the level it starts from (baseLevel): per band
// (jet − ⟨level⟩) / relaxation spread back to the cells, plus the eddies' tendency (eddyFeeding of
// the published eddies), plus the damping with `poisson`'s K; the friction rates. Throws
// std::invalid_argument unless closuresFit, before `forcing` changes.
void fillClosures(SphereClosures& closures, const SphereGrid& grid, const SpherePoisson& poisson, const SphereFlow& flow,
                  SphereForcing& forcing);

// One window of the flow forced by the closures, its steps held to the same damping's bound: sets
// hour.stiffest to the damping's stiffness, then advanceSphereHour with fillClosures' tendency as the
// source and the closures' friction fixed for the window, read in place. The eddies are the window's
// context, fixed through it: once `since` has reached eddyEvery, the band of ζ at the window's start
// (filterEddyBand), else the published ones; with those, the same forcing as fillClosures', bit for
// bit. A refresh is published (eddy, and `since` set to the window) only with the window taken; a
// window taken without one adds its length to `since`. Refused or thrown, the window publishes
// nothing, so the same call repeats the same refresh and the same tries. Throws std::invalid_argument
// unless closuresFit, before anything changes, hour.stiffest included; checked once a window, and
// hour.stiffest set only after the refresh and the eddies' tendency are made.
// Once the window has begun, hour.stiffest stays set after its refusal or an exception from within
// it (the same call repeats the same tries).
[[nodiscard]] bool advanceClosedHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, SphereClosures& closures);

} // namespace derivean::sim
