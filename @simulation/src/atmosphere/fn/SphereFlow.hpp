#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// Earth's rotation rate at a planet age, Ω = 2π / sidereal day, from the astronomy slice (S2):
// 7.292e-5 1/s today, faster in the deep past.
double rotationRateAt(double age);   // age: yr since the hot initial body

SphereFlow makeSphereFlow(const SphereGrid& grid, double rotation);

// Starts the flow from a relative vorticity (its area mean, which no wind on a closed sphere can
// have, is reported by the inversion and left out of the wind): both leapfrog levels, the stream
// function and the edge fluxes.
void setVorticity(SphereFlow& flow, const SphereGrid& grid, SpherePoisson& poisson, const std::vector<double>& vorticity);

// One step of ∂ζ/∂t = −∇·(v (ζ + f)) in flux form (Charney, Fjørtoft & von Neumann 1950): each edge
// carries its volume flux times the absolute vorticity at the edge's midpoint, applied to both cells
// with opposite signs, so the area integral of ζ + f is kept to rounding. The edge value is the mean
// of the two cells moved to the midpoint by the edge gradient (SphereEdge::offset), because on face
// edges the chord's midpoint, where the plain mean sits, is O(h) from the edge's. Neither energy nor
// enstrophy is conserved: unforced, both drift at the scheme's error, and grid-scale vorticity grows
// slowly at the cube vertices, so the closures must damp it (docs/handoff/atmosphere-sphere.md).
// Leapfrog in time with the Robert–Asselin–Williams filter (Robert 1966, Asselin
// 1972, Williams 2009), as the flat-world circulation; the first step is a forward step. Then the new
// ψ is found with the last one as the first guess.
void stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, SpherePoisson& poisson, double step);

// The largest share of a cell's content the fluxes take out of it in `step` (the outflow Courant
// number summed over its edges). The tests step at 0.4 (flow) and 0.45 (tracer).
double outflowCourant(const SphereGrid& grid, const std::vector<double>& flux, double step);

// Moves a passive tracer (amount per area) one forward step with the edge fluxes: upwind plus a
// van Leer-limited (van Leer 1974) Lax–Wendroff correction along the grid line (Sweby 1984), so the
// area integral is kept to rounding and a uniform tracer stays uniform; below Courant 0.5 the tests
// see no value leave the range it started in.
void advectTracer(const SphereGrid& grid, const std::vector<double>& flux, double step, std::vector<double>& tracer,
                  std::vector<double>& work);

} // namespace derivean::sim
