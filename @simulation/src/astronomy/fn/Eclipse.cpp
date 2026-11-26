#include "astronomy/fn/Eclipse.hpp"

#include "astronomy/constant/Sky.hpp"
#include "astronomy/fn/Place.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace derivean::sim {
namespace {

constexpr double ShadowEnlargement = 1.02;

double separation(Vec3 a, Vec3 b) { return std::atan2(length(cross(a, b)), dot(a, b)); }

// Area shared by two discs of radii a and b whose centres lie `distance` apart (small angles, flat).
double overlap(double a, double b, double distance) {
    if (distance >= a + b) return 0;
    if (distance <= std::abs(a - b)) return std::numbers::pi * std::min(a, b) * std::min(a, b);
    const double alpha = std::acos(std::clamp((distance * distance + a * a - b * b) / (2 * distance * a), -1.0, 1.0));
    const double beta = std::acos(std::clamp((distance * distance + b * b - a * a) / (2 * distance * b), -1.0, 1.0));
    return a * a * (alpha - std::sin(2 * alpha) / 2) + b * b * (beta - std::sin(2 * beta) / 2);
}

} // namespace

double sunCoverage(const Sky& sky, Vec3 observer) {
    const Vec3 sun = normalized(sky.sun - observer), moon = moonDirection(sky, observer);
    const double sunRadius = std::asin(SunRadius / length(sky.sun - observer));
    const double moonRadius = moonAngularRadius(sky, observer);
    return overlap(sunRadius, moonRadius, separation(sun, moon)) / (std::numbers::pi * sunRadius * sunRadius);
}

SolarShadow solarShadow(const Sky& sky) {
    const Vec3 axis = sky.moon - sky.sun;
    const double span = length(axis);
    const Vec3 along = axis * (1 / span);
    const double beyond = dot(sky.moon * -1.0, along);   // from the Moon to the fundamental plane
    if (beyond <= 0) return {std::numeric_limits<double>::infinity(), 0, 0, false};   // the shadow falls away from the Earth
    const Vec3 offset = sky.moon + along * beyond;       // the axis's point in the plane
    const double penumbra = std::asin((SunRadius + MoonRadius) / span), umbra = std::asin((SunRadius - MoonRadius) / span);
    return {
        length(offset),
        MoonRadius / std::cos(penumbra) + beyond * std::tan(penumbra),
        MoonRadius / std::cos(umbra) - beyond * std::tan(umbra),
        offset.z > 0,
    };
}

LunarShadow lunarShadow(const Sky& sky, double planetRadius) {
    const double moonDistance = length(sky.moon), sunDistance = length(sky.sun);
    const double moonRadius = std::asin(MoonRadius / moonDistance), sunRadius = std::asin(SunRadius / sunDistance);
    const double parallaxes = std::asin(planetRadius / moonDistance) + std::asin(planetRadius / sunDistance);
    const double umbra = ShadowEnlargement * (parallaxes - sunRadius), penumbra = ShadowEnlargement * (parallaxes + sunRadius);
    const double offset = separation(sky.moon, sky.sun * -1.0);
    return {
        (penumbra + moonRadius - offset) / (2 * moonRadius),
        (umbra + moonRadius - offset) / (2 * moonRadius),
        overlap(moonRadius, umbra, offset) / (std::numbers::pi * moonRadius * moonRadius),
    };
}

} // namespace derivean::sim
