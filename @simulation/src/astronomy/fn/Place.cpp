#include "astronomy/fn/Place.hpp"

#include "astronomy/constant/Sky.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace derivean::sim {
namespace {

constexpr double Turn = 2 * std::numbers::pi;

double turns(double angle) { return angle / Turn - std::floor(angle / Turn); }

// Longitude in the Earth frame.
double longitudeOf(Vec3 v) { return std::atan2(v.y, v.x); }

} // namespace

Vec3 sunDirection(const Sky& sky) { return normalized(sky.sun); }

Vec3 moonDirection(const Sky& sky, Vec3 observer) { return normalized(sky.moon - observer); }

double sunElevation(const Sky& sky, Vec3 up) { return std::asin(std::clamp(dot(up, sunDirection(sky)), -1.0, 1.0)); }

double moonElevation(const Sky& sky, Vec3 observer) {
    return std::asin(std::clamp(dot(normalized(observer), moonDirection(sky, observer)), -1.0, 1.0));
}

double sunDeclination(const Sky& sky) { return std::asin(std::clamp(sunDirection(sky).z, -1.0, 1.0)); }

double sunAngularRadius(const Sky& sky) { return std::asin(SunRadius / length(sky.sun)); }

double moonAngularRadius(const Sky& sky, Vec3 observer) { return std::asin(MoonRadius / length(sky.moon - observer)); }

// The hour angle is how far west of the meridian the Sun stands.
double solarTime(const Sky& sky, Vec3 up) { return 24 * turns(longitudeOf(up) - longitudeOf(sky.sun) + std::numbers::pi); }

double meanSolarTime(const Sky& sky, Vec3 up) {
    // The mean Sun moves evenly along the equator of date at the mean longitude.
    return 24 * turns(longitudeOf(up) - (sky.meanSunLongitude - sky.rotation) + std::numbers::pi);
}

double equationOfTime(const Sky& sky) {
    const double rightAscension = longitudeOf(sky.sun) + sky.rotation;
    return std::remainder(sky.meanSunLongitude - rightAscension, Turn) / Turn * JulianDay;
}

double irradiance(const Sky& sky) {
    const double distance = length(sky.sun);
    return sky.deep.luminosity / (4 * std::numbers::pi * distance * distance);
}

double insolation(const Sky& sky, Vec3 up) { return irradiance(sky) * std::max(0.0, dot(up, sunDirection(sky))); }

double dailyMeanInsolation(const Sky& sky, Vec3 up) {
    // The Sun's height over the day is sin φ sin δ + cos φ cos δ cos h at hour angle h.
    const double sinDeclination = sunDirection(sky).z, sinLatitude = up.z;
    const double cosDeclination = std::sqrt(std::max(0.0, 1 - sinDeclination * sinDeclination));
    const double cosLatitude = std::sqrt(std::max(0.0, 1 - sinLatitude * sinLatitude));
    const double steady = sinLatitude * sinDeclination, swing = cosLatitude * cosDeclination;
    // cos h₀ = −steady / swing, clamped: at or past the polar circle the Sun stays up or down.
    if (steady >= swing) return irradiance(sky) * steady;
    if (steady <= -swing) return 0;
    const double halfDay = std::acos(-steady / swing);
    const double sinHalfDay = std::sqrt(std::max(0.0, 1 - (steady / swing) * (steady / swing)));
    return irradiance(sky) / std::numbers::pi * (halfDay * steady + swing * sinHalfDay);
}

} // namespace derivean::sim
