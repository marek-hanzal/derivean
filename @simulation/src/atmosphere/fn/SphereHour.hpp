#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SphereForcing.hpp"
#include "atmosphere/type/SphereHour.hpp"
#include "atmosphere/type/SpherePoisson.hpp"
#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

namespace derivean::sim {

// Moves the flow over one window (SphereHour), the forcing of each step from `source`
// (SphereForcingSource), called before every step of every try. Returns true with the window taken;
// false when every try was refused, with the flow's state as it was before the window, its history
// included (SphereFlowStart: what SphereFlow names its state; its work space is the last try's), the
// reasons in hour.refusals and hour.steps unchanged, so the same call repeats the same tries. A refused
// try leaves nothing behind that a step reads: the next one computes what a clean run from the
// window's start with its steps computes, bit for bit (the source reading only the flow's state and a
// context fixed for the window). A window shorter than `shortest` is refused (`Shortest`, no steps), and one
// whose `window / shortest` steps the damping's bound (`stiffest`) refuses (`Damping`), before the
// flow or the steps change; an exception from a try puts the flow's state and the hour's controller
// back as they came in and passes on (hour.start and hour.filled are work space).
[[nodiscard]] bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, const SphereForcingSource& source);

// The same with a friction rate per cell fixed for the window (the closures', fn/SphereClosures),
// checked once a window (admissibleFriction) and read in place, not copied into each step's forcing
// (Neon DREV-NEON-065); the source fills only the tendency, and one that fills a friction too is
// refused (`Forcing`).
[[nodiscard]] bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, const SphereForcingSource& source, const std::vector<double>& friction);

// The same with one forcing held through the window (an empty one: no closures).
[[nodiscard]] bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, const SphereForcing& forcing);

} // namespace derivean::sim
