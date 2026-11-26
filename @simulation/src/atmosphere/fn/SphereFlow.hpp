#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// Earth's rotation rate at a planet age, Ω = 2π / sidereal day, from the astronomy slice (S2):
// 7.292e-5 1/s today, faster in the deep past.
double rotationRateAt(double age);   // age: yr since the hot initial body

SphereFlow makeSphereFlow(const SphereGrid& grid, double rotation);

// Starts the flow from a relative vorticity (its area mean, which no wind on a closed sphere can
// have, is reported by the inversion and left out of the wind): both leapfrog levels, the stream
// function and the edge fluxes, and a restart. Returns false and leaves the flow as it was when the
// inversion does not converge (flow.inversion says how far it got).
[[nodiscard]] bool setVorticity(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                                const std::vector<double>& vorticity);

// Sets the step (s, > 0). A different step restarts the history: the next step is a forward step
// from the current vorticity, as after setVorticity, because the leapfrog and its filter take the
// last two levels as one step apart. The same step changes nothing.
void setStep(SphereFlow& flow, double step);

// One step of ∂ζ/∂t = −∇·(v (ζ + f)) in flux form (Charney, Fjørtoft & von Neumann 1950): each edge
// carries its volume flux times the absolute vorticity at the edge's midpoint, applied to both cells
// with opposite signs, so the area integral of ζ + f is kept to rounding. The edge value is the mean
// of the two cells moved to the midpoint by the edge gradient (EdgeStencil::offset), because on face
// edges the chord's midpoint, where the plain mean sits, is O(h) from the edge's. Neither energy nor
// enstrophy is conserved: unforced, both drift at the scheme's error, and grid-scale vorticity grows
// slowly at the cube vertices, so the closures must damp it (docs/handoff/atmosphere-sphere.md).
// Leapfrog in time with the Robert–Asselin–Williams filter (Robert 1966, Asselin 1972, Williams 2009)
// at flow.step, as the flat-world circulation; the first step after a restart is a forward step.
// The candidate ψ is found with the last one as the first guess, and the step is accepted only if
// that converges: then it returns true; otherwise it returns false and leaves the state as it was,
// so a retry (with a larger limit, say) computes the same step a clean run does. Without a positive
// step (setStep not called) it returns false and does nothing.
[[nodiscard]] bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson);

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
