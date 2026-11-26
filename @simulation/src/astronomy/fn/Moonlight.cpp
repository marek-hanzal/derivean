#include "astronomy/fn/Moonlight.hpp"

#include "astronomy/fn/Frame.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace derivean::sim {

double lunarAge(const Sky& sky) {
    const Vec3 sun = inertialToEcliptic(sky.sunInertial), moon = inertialToEcliptic(sky.moonInertial);
    const double elongation = (std::atan2(moon.y, moon.x) - std::atan2(sun.y, sun.x)) / (2 * std::numbers::pi);
    return elongation - std::floor(elongation);
}

double moonIllumination(const Sky& sky, Vec3 observer) {
    const double phase = dot(moonSunward(sky), normalized(observer - sky.moon));
    return (1 + std::clamp(phase, -1.0, 1.0)) / 2;
}

Vec3 moonSunward(const Sky& sky) { return normalized(sky.sun - sky.moon); }

} // namespace derivean::sim
