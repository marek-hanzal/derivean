#include "astronomy/fn/Tide.hpp"

#include "astronomy/constant/Sky.hpp"

#include <cmath>
#include <stdexcept>

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

void wetTide(const Sky& sky, std::span<const Vec3> wet, std::span<const double> areas, std::span<double> heights) {
    if (wet.size() != areas.size() || wet.size() != heights.size()) throw std::invalid_argument("wetTide: wet, areas and heights differ in size");
    double volume = 0, area = 0;
    for (std::size_t i = 0; i < wet.size(); ++i) {
        heights[i] = equilibriumTide(sky, wet[i]);
        volume += heights[i] * areas[i];
        area += areas[i];
    }
    const double level = area > 0 ? volume / area : 0;
    for (double& height : heights) height -= level;
}

} // namespace derivean::sim
