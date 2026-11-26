#pragma once

#include "astronomy/type/Sky.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// [0, 1): the Moon's age in the synodic month, 0 at new and 0.5 at full moon, from the difference of
// the two bodies' ecliptic longitudes.
double lunarAge(const Sky& sky);

// Share [0, 1] of the Moon's disc lit for an observer (Earth frame, m), from the phase angle
// Sun–Moon–observer.
double moonIllumination(const Sky& sky, Vec3 observer);

// Unit direction from the Moon towards the Sun in the Earth frame: the renderer lights the disc from it.
Vec3 moonSunward(const Sky& sky);

} // namespace derivean::sim
