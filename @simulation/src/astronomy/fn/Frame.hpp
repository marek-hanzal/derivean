#pragma once

#include "astronomy/type/DeepTime.hpp"
#include "astronomy/type/Instant.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// The Earth's orientation. The spin axis keeps the obliquity and precesses about the pole of the
// J2000 ecliptic; the ecliptic itself is held (its own drift is 47" per century), and nutation
// (±17" in longitude, ±9" in obliquity) is left out.

// From the J2000 ecliptic to the mean equator of J2000 (the inertial frame of `Sky`).
Vec3 eclipticToInertial(Vec3 ecliptic);
Vec3 inertialToEcliptic(Vec3 inertial);

// From the J2000 ecliptic to the Earth frame: precess to the equinox of date, tilt to its equator,
// turn by the sidereal angle.
Vec3 eclipticToEarth(Vec3 ecliptic, double precession, double rotation);

// Greenwich mean sidereal angle (Meeus 1998, 12.4), rad in [0, 2π). Up to the epoch the Earth turns
// at today's rate (with UT1 = TT − ΔT); from it, at the sidereal day of `deep`.
double rotationAt(const Instant& instant, const DeepTime& deep);

} // namespace derivean::sim
