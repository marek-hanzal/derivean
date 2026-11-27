#pragma once

#include "astronomy/type/Sky.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// The sky at a place. The caller gives the place as geometry's unit `up` in the Earth frame, or as its
// Earth-frame position in metres (`observer`) where the distance matters: the Moon sits 60 Earth radii
// away, so its direction shifts across the planet by 0.95° on average and up to 1.02° at perigee. Angles
// are radians. Refraction belongs to the air, not here: elevations are geometric.

Vec3 sunDirection(const Sky& sky);                          // unit; the Sun's parallax (8.8") is left out
Vec3 moonDirection(const Sky& sky, Vec3 observer);          // unit, from the observer
double sunElevation(const Sky& sky, Vec3 up);
double moonElevation(const Sky& sky, Vec3 observer);
double sunDeclination(const Sky& sky);                      // against the equator of date

// Radius of the disc as seen.
double sunAngularRadius(const Sky& sky);
double moonAngularRadius(const Sky& sky, Vec3 observer);

// Hours in [0, 24). Apparent solar time is 12 when the Sun crosses the meridian; mean solar time runs
// evenly with the mean Sun. Both follow the longitude; at a pole, which has none, they read Greenwich's.
double solarTime(const Sky& sky, Vec3 up);
double meanSolarTime(const Sky& sky, Vec3 up);

// s, apparent minus mean solar time (Meeus 1998, ch. 28), the same everywhere: −14 min in February,
// +16 min in November.
double equationOfTime(const Sky& sky);

// W/m² above the air: across the Sun's direction at the Earth's distance (1/r²: +3.4 % at
// perihelion), and on a level surface at the place (zero below the horizon).
double irradiance(const Sky& sky);
double insolation(const Sky& sky, Vec3 up);

// W/m², the mean of `insolation` over one solar day (the hour angle through 2π) with the Sun's
// declination and distance held at the instant's (Berger 1978): (S / π) (h₀ sin φ sin δ + cos φ cos δ
// sin h₀), where the half-day h₀ is the hour angle at which the Sun crosses the horizon, π in polar day
// and 0 in polar night. Over a real day the declination moves by up to 0.4° and the distance by
// 0.03 %, which this leaves out. `up` is a unit direction (its z is sin φ). The half-day's acos is
// libm, called per place and step, so across platforms it is in the determinism queue.
double dailyMeanInsolation(const Sky& sky, Vec3 up);

} // namespace derivean::sim
