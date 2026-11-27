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

// The forcing of the next step from the level it starts from (baseLevel): per band
// (jet − ⟨level⟩) / relaxation spread back to the cells, plus the damping with `stiffness`, the K it
// was made from; the friction rates.
void fillClosures(SphereClosures& closures, const SphereGrid& grid, const StencilMatrix& stiffness, const SphereFlow& flow,
                  SphereForcing& forcing);

// One window of the flow forced by the closures, its steps held to the same damping's bound: sets
// hour.stiffest to the damping's stiffness, then advanceSphereHour with fillClosures as the source.
// Throws std::invalid_argument, before anything changes, unless `poisson` is the one the closures
// were made from. hour.stiffest stays set after a refusal or an exception (the same call repeats the
// same tries).
[[nodiscard]] bool advanceClosedHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, SphereClosures& closures);

} // namespace derivean::sim
