#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/type/SphereForcing.hpp"
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
// The candidate ψ is found from guessStream's first guess (the accepted ψ of the last three steps
// projected on the new source, Fischer 1998; restarts drop the older ones), and the step is accepted
// only if that converges: then it returns true; otherwise it returns false and leaves the state as it was,
// so a retry (with a larger limit, say) computes the same step a clean run does. Without a positive
// step (setStep not called) it returns false and does nothing.
[[nodiscard]] bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson);

// The same step with the closures' forcing (SphereForcing; S4 plan, binding correction 1). Over the
// span s (2Δt, or Δt in a forward step) the level before (ζₙ in a forward step) takes
// x = before + s · (advection + tendency), friction keeps keep = 1 / (1 + s · r) of it, and the area
// mean is taken out where it is kept: next = keep · (x − c), c = Σ A keep x / Σ A keep, so
// Σ A next = 0 to rounding whatever r and the tendency are. Then the filter and the inversion as
// above. Both time levels keep a zero mean once they have one: setVorticity keeps the mean it is
// given in ζ, and from such a start the filter carries it out of the older level within steps. A
// forcing with both vectors empty is the step above, bit for bit; one that is not admissible
// (admissibleForcing) is refused (false, nothing changed).
[[nodiscard]] bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                                  const SphereForcing& forcing);

// Whether stepSphereFlow takes `forcing` on a grid of `cells`: each vector empty or one value per
// cell, every value finite, every friction rate at least zero.
bool admissibleForcing(const SphereForcing& forcing, size_t cells);

} // namespace derivean::sim
