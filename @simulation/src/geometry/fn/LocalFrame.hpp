#pragma once

#include "geometry/type/Frame.hpp"

namespace derivean::sim {

// A flat east-north-up frame anchored at an ECEF point, for voxel physics in a bounded neighbourhood.
LocalFrame anchorAt(Vec3 ecef);
Vec3 localOf(const LocalFrame& frame, Vec3 ecef);     // (east, north, up) metres from the origin
Vec3 ecefOfLocal(const LocalFrame& frame, Vec3 local);

// The error budget of treating the tangent plane as the sphere, at horizontal distance d from the
// anchor, for a plane at radius r from the centre (the anchor's PlanetRadius + height):
//   height:   how far the plane rises above the sphere of radius r there, sqrt(r² + d²) − r ≈ d²/2r
//             (7.85 cm at 1 km, 785 m at 100 km);
//   tilt:     the angle between the frame's up and true up, atan(d / r) (0.009° at 1 km);
//   distance: planar distance minus the arc it stands for, d − r·atan(d / r) ≈ d³/3r² (8.2 µm at 1 km,
//             8.2 m at 100 km).
struct TangentError {
    double height{};    // m
    double tilt{};      // rad
    double distance{};  // m
};
TangentError tangentErrorAt(double distance, double radius);

// The largest horizontal distance whose height error stays within a tolerance: sqrt(t (2r + t)).
double tangentReach(double tolerance, double radius);

} // namespace derivean::sim
