#include "measure/fn/Scout.hpp"

#include "measure/constant/Angle.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr double EyeHeight = 1.8;   // m above the ground a standing viewer looks from

float groundAt(const Mosaic& mosaic, Vec2 at) {
    const int x = std::clamp(cellX(mosaic, at), 0, mosaic.side() - 1), z = std::clamp(cellZ(mosaic, at), 0, mosaic.side() - 1);
    return mosaic.top(x, z);
}

CameraPose poseFrom(Vec2 eye, double eyeY, Vec2 target, double targetY) {
    const Vec2 to = target - eye;
    return {eye.x, eyeY, eye.z, std::atan2(-to.x, -to.z) * DegreesPerRadian, std::atan2(targetY - eyeY, std::max(1.0, length(to))) * DegreesPerRadian};
}

// True when the ground between the two points stays under the straight line that joins them.
bool clear(const Mosaic& mosaic, Vec2 eye, double eyeY, Vec2 target, double targetY) {
    const int steps = std::max(2, int(length(target - eye) / mosaic.cell));
    for (int k = 1; k < steps; ++k) {
        const double t = double(k) / steps;
        if (groundAt(mosaic, eye + (target - eye) * t) > mix(eyeY, targetY, t)) return false;
    }
    return true;
}

} // namespace

void frameSite(const Mosaic& mosaic, ScoutSite& site, double airBack, double airUp) {
    const double targetY = double(site.ground) + site.lookUp;
    // The viewer stands where the detector suggested, or as near to that bearing as dry ground with a clear view allows.
    const Vec2 wanted = site.stand - site.at;
    const double distance = std::max(mosaic.cell, length(wanted));
    const double bearing = std::atan2(wanted.z, wanted.x);
    Vec2 stand = site.stand;
    for (const double turn : {0.0, 0.4, -0.4, 0.8, -0.8, 1.3, -1.3, 2.0, -2.0, 3.14}) {
        const Vec2 tried = site.at + Vec2{std::cos(bearing + turn), std::sin(bearing + turn)} * distance;
        const int x = cellX(mosaic, tried), z = cellZ(mosaic, tried);
        if (!mosaic.contains(x, z) || mosaic.flooded(x, z)) continue;
        if (!clear(mosaic, tried, mosaic.ground.at(x, z) + EyeHeight, site.at, targetY)) continue;
        stand = tried;
        break;
    }
    site.stand = stand;
    site.groundShot = poseFrom(stand, groundAt(mosaic, stand) + EyeHeight, site.at, targetY);

    // The air shot backs away along the same bearing and clears everything under its line of flight.
    const Vec2 back = normalized(stand - site.at);
    const Vec2 eye = site.at + back * airBack;
    double highest = site.ground;
    for (int k = 0; k <= 16; ++k) highest = std::max(highest, double(groundAt(mosaic, site.at + back * (airBack * k / 16.0))));
    site.airShot = poseFrom(eye, highest + airUp, site.at, targetY);
}

} // namespace derivean::sim
