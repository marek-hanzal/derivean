#pragma once

// Shared helpers of the astronomy checks. Places are given the way the references give them
// (geodetic latitude and east longitude in degrees); the up of a geodetic latitude is the ellipsoid's
// normal, which is what JPL Horizons measures elevation against. Only the tests turn latitude and
// longitude into an up: the frame belongs to geometry.
#include "astronomy/fn/Sky.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/type/Vec3.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>

namespace derivean::sim::astronomytest {

inline constexpr double Degree = std::numbers::pi / 180;
inline constexpr double Minute = 60.0;
inline constexpr double Hour = 3600.0;

inline void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Astronomy: " << message << " (" << value << ")\n";
    std::exit(1);
}

inline Vec3 upAt(double latitude, double longitude) {
    return {std::cos(latitude * Degree) * std::cos(longitude * Degree), std::cos(latitude * Degree) * std::sin(longitude * Degree),
            std::sin(latitude * Degree)};
}

inline Vec3 observerAt(double latitude, double longitude) { return upAt(latitude, longitude) * PlanetRadius; }

// Today's sky at a Julian date in TT.
inline Instant at(double julianDate, double seconds = 0) { return {julianDate, seconds, PresentAge}; }

inline Sky skyOn(double julianDate, double seconds = 0) { return skyAt(at(julianDate, seconds)); }

// Unit vector of a right ascension and declination in degrees.
inline Vec3 equatorial(double rightAscension, double declination) {
    return {std::cos(declination * Degree) * std::cos(rightAscension * Degree), std::cos(declination * Degree) * std::sin(rightAscension * Degree),
            std::sin(declination * Degree)};
}

inline double degreesBetween(Vec3 a, Vec3 b) { return std::atan2(length(cross(a, b)), dot(a, b)) / Degree; }

// Seconds after `from` where `f` crosses `level` upwards, by bisection inside [from, from + span].
template <class F>
double rising(F f, double from, double span, double level) {
    double low = from, high = from + span;
    for (int i = 0; i < 60; ++i) {
        const double middle = (low + high) / 2;
        (f(middle) < level ? low : high) = middle;
    }
    return (low + high) / 2;
}

} // namespace derivean::sim::astronomytest
