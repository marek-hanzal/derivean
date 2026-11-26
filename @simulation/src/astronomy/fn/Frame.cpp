#include "astronomy/fn/Frame.hpp"

#include "astronomy/constant/Sky.hpp"

#include <cmath>
#include <numbers>

namespace derivean::sim {
namespace {

constexpr double Degree = std::numbers::pi / 180;

Vec3 aboutX(Vec3 v, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

Vec3 aboutZ(Vec3 v, double angle) {
    const double c = std::cos(angle), s = std::sin(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

} // namespace

Vec3 eclipticToInertial(Vec3 ecliptic) { return aboutX(ecliptic, Obliquity * Degree); }

Vec3 inertialToEcliptic(Vec3 inertial) { return aboutX(inertial, -Obliquity * Degree); }

Vec3 eclipticToEarth(Vec3 ecliptic, double precession, double rotation) {
    // Longitudes of date exceed J2000 ones by the precession; the Earth frame turns east with the sky
    // fixed, so the sky turns west in it.
    return aboutZ(aboutX(aboutZ(ecliptic, precession), Obliquity * Degree), -rotation);
}

double rotationAt(const Instant& instant, const DeepTime& deep) {
    const double days = instant.epoch - J2000 - DeltaT / JulianDay;
    const double today = 360.0 * JulianDay / SiderealDay;   // degrees per day
    const double epoch = std::fmod(GreenwichSiderealJ2000 + today * days, 360.0) * Degree;
    const double turned = std::fmod(instant.seconds / deep.siderealDay, 1.0) * 2 * std::numbers::pi;
    const double angle = std::fmod(epoch + turned, 2 * std::numbers::pi);
    // fmod of a tiny negative angle plus 2π rounds to 2π itself.
    return angle < 0 ? std::fmod(angle + 2 * std::numbers::pi, 2 * std::numbers::pi) : angle;
}

} // namespace derivean::sim
