#pragma once

#include "tectonics/type/Euler.hpp"

namespace derivean::sim {

// The epoch at `time`: n = lattice² plate sites (9 to 25, as on the flat planet) on a jittered
// Fibonacci lattice (González 2010) turned at random, each with a reach, a pole uniform on the sphere
// and the draw of its speed. Euler vectors are set later (setEuler), once the plate's crust is known.
EulerEpoch eulerEpochAt(uint64_t worldSeed, double time);

// The plate whose weighted Voronoi region holds a unit direction, after the epoch's 3D warp.
int plateAt(const EulerEpoch& epoch, Vec3 direction);

// Slow swell of the surface above upwellings and sag above downwellings, metres; fixed in the mantle.
double swellAt(const EulerEpoch& epoch, Vec3 direction);

// The target RMS speed of a plate (m/yr) from the share of its area that is continental at the epoch's
// start. Earth: mix(65.5, 22.8, share) mm/yr × draw, the Pacific and Eurasia in NNR-MORVEL56 (Argus et
// al. 2011). Today: the flat history's 4 to 30 mm/yr by hash.
double targetSpeed(const EulerEpoch& epoch, int plate, double continentalShare, PlateSpeeds speeds);

// Sets every plate's `speed` and `euler`: ω = pole · v / (R · sqrt(<sin² γ>)), where γ is the angle
// between the pole and the plate's crust and the mean is weighted by area, so that the plate's crust
// moves at RMS speed v. `meanSin2` and `continentalShare` are per plate.
void setEuler(EulerEpoch& epoch, const std::vector<double>& meanSin2, const std::vector<double>& continentalShare, PlateSpeeds speeds);

// Velocity of the surface at a unit direction under an Euler vector: R (ω × x), m/yr.
Vec3 velocityAt(Vec3 euler, Vec3 direction);

// How two plates meet at a unit direction x of a contact: v_rel = R (ω_p − ω_q) × x, convergence
// v_rel · n̂ (n̂ the unit tangent from p towards q; positive closes) and shear v_rel · (x × n̂), m/yr.
struct Closing {
    double convergence{}, shear{};
};
Closing closingAt(Vec3 own, Vec3 other, Vec3 direction, Vec3 normal);

void addInertia(Inertia& inertia, Vec3 direction, double area);
Vec3 apply(const Inertia& inertia, Vec3 v);

// The no-net-rotation frame (Argus et al. 2011) after damping (Codex V5): the rotation ω_net = M⁻¹
// Σ_p I_p ω̃_p with M = Σ_p I_p, solved exactly as 3 × 3 because pieces do not tile the sphere exactly,
// removed from every damped vector. Returns ω̂_p = ω̃_p − ω_net; `net` receives ω_net.
std::vector<Vec3> withoutNetRotation(const std::vector<Vec3>& damped, const std::vector<Inertia>& inertia, Vec3* net = nullptr);

} // namespace derivean::sim
