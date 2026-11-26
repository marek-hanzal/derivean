#pragma once

#include "astronomy/type/DeepTime.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// The Sun and the Moon at one instant. Positions are geocentric, in metres.
//   - Earth frame: geometry's ECEF (Z to the north pole, X to latitude 0 longitude 0, Y to 90° E).
//   - Inertial frame: the mean equator and equinox of J2000 (EME2000, within 0.02" of ICRF).
// The Sun's position is geometric: light time (8.3 min) and aberration (20.5") are left out, which
// moves it by under 0.006°.
struct Sky {
    Vec3 sun, moon;                     // Earth frame
    Vec3 sunInertial, moonInertial;     // inertial frame
    double rotation{};                  // rad, Greenwich mean sidereal angle against the equinox of date
    double precession{};                // rad, general precession in longitude since J2000
    double meanSunLongitude{};          // rad, the mean Sun's longitude against the equinox of date
    DeepTime deep;
};

} // namespace derivean::sim
