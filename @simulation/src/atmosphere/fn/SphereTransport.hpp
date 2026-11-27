#pragma once

#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// What the barotropic flow's edge fluxes (SphereFlow::flux) carry: their Courant number, which
// bounds the wind step and the transport, and a passive tracer moved with them.

// The largest share of a cell's content the fluxes take out of it in `step` (the outflow Courant
// number summed over its edges). The tests step at 0.4 (flow) and 0.45 (tracer).
double outflowCourant(const SphereGrid& grid, const std::vector<double>& flux, double step);

// Moves a passive tracer (amount per area) one forward step with the edge fluxes: upwind plus a
// van Leer-limited (van Leer 1974) Lax–Wendroff correction along the grid line (Sweby 1984), so the
// area integral is kept to rounding and a uniform tracer stays uniform; below Courant 0.5 the tests
// see no value leave the range it started in.
void advectTracer(const SphereGrid& grid, const SphereStencil& stencil, const std::vector<double>& flux, double step,
                  std::vector<double>& tracer, std::vector<double>& work);

} // namespace derivean::sim
