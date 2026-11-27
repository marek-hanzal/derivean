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
// false when every try was refused, with the flow as it was before the window, its history
// included, the reasons in hour.refusals and hour.steps unchanged, so the same call repeats the same
// tries. A refused try leaves nothing behind: the next one computes what a clean run from the
// window's start with its steps computes, bit for bit (the source reading only the flow and a context
// fixed for the window). A window shorter than `shortest` is refused (`Shortest`, no steps), and one
// whose `window / shortest` steps the damping's bound (`stiffest`) refuses (`Damping`), before the
// flow or the steps change; an exception from a try puts the flow and the hour back as they came in
// and passes on.
[[nodiscard]] bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, const SphereForcingSource& source);

// The same with one forcing held through the window (an empty one: no closures).
[[nodiscard]] bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, const SphereForcing& forcing);

} // namespace derivean::sim
