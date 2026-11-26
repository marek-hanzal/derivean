#pragma once

#include "astronomy/type/DeepTime.hpp"
#include "astronomy/type/Instant.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// Mean arguments of the Sun's and the Moon's orbits, in radians. Longitudes are measured in the
// inertial ecliptic and equinox of J2000.
struct MeanElements {
    double sunAnomaly{};        // M
    double sunLongitude{};      // L☉
    double moonLongitude{};     // L'
    double moonAnomaly{};       // M'
    double moonNode{};          // Ω, ascending node
    double precession{};        // general precession in longitude since J2000
};

// Up to the epoch the arguments advance at today's rates; from it, at the rates of `deep`.
MeanElements meanElementsAt(const Instant& instant, const DeepTime& deep);

// Eccentric anomaly E of a mean anomaly M: E − e sin E = M.
double eccentricAnomaly(double meanAnomaly, double eccentricity);

// The Sun from the Earth–Moon barycentre: a Kepler ellipse in the J2000 ecliptic, m.
Vec3 sunFromBarycentre(const MeanElements& elements);

// The Moon from the Earth in the J2000 ecliptic, m: a Kepler ellipse of the given axis, eccentricity
// and inclination, its node and perigee turning, plus the Sun's largest perturbations (evection,
// variation, annual equation and their kin, Meeus tables 47.A and 47.B down to 0.01°). The
// perturbations keep today's amplitudes at every age.
Vec3 moonFromEarth(const MeanElements& elements, const DeepTime& deep);

} // namespace derivean::sim
