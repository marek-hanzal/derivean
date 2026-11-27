#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SphereForcing.hpp"
#include "atmosphere/type/SphereHour.hpp"
#include "atmosphere/type/SpherePoisson.hpp"
#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

namespace derivean::sim {

// Moves the flow over one window with `forcing` held through it (SphereHour). Returns true with the
// window taken; false when every try was refused, with the flow as it was before the window, its
// history included, and the reasons in hour.refusals. A refused try leaves nothing behind: the next
// try computes what a clean run from the window's start with its steps computes, bit for bit.
[[nodiscard]] bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                                     SpherePoisson& poisson, const SphereForcing& forcing);

} // namespace derivean::sim
