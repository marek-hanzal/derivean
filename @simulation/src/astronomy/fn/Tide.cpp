#include "astronomy/fn/Tide.hpp"

#include "astronomy/constant/Sky.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

double bodyPotential(double gm, Vec3 body, Vec3 observer) {
    const double distance = length(body), radius = length(observer);
    const double c = dot(body, observer) / (distance * radius), ratio = radius / distance;
    const double second = (3 * c * c - 1) / 2, third = (5 * c * c * c - 3 * c) / 2;
    return gm / distance * ratio * ratio * (second + ratio * third);
}

} // namespace

double tidePotential(const Sky& sky, Vec3 observer) {
    return bodyPotential(MoonGM, sky.moon, observer) + bodyPotential(SunGM, sky.sun, observer);
}

double equilibriumTide(const Sky& sky, Vec3 observer) {
    const double radius = length(observer);
    return tidePotential(sky, observer) * radius * radius / EarthGM;
}

} // namespace derivean::sim
