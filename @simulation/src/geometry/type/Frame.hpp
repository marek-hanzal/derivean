#pragma once

#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// Geocentric latitude and longitude in radians, east positive. Only for reading and for the user:
// positions are faces and cells, or ECEF.
struct LatLon {
    double latitude{};
    double longitude{};
};

// An orthonormal right-handed tangent basis at a point of the sphere: `first × second = up`. For the
// geographic frame first is east and second north; for a face frame first follows the face's +u grid
// line and second is up × first.
struct SurfaceFrame {
    Vec3 first;
    Vec3 second;
    Vec3 up;
};

// A tangent vector by its components in a SurfaceFrame.
struct TangentVector {
    double first{};
    double second{};
};

// How carrying a vector from one cell's frame to another turns its components: a rotation by the angle
// whose cosine and sine these are. Fixed per pair of cells, so a solver can keep one per edge.
struct FrameRotation {
    double cosine{1};
    double sine{};
};

// A flat local frame anchored on the sphere for voxel physics: Cartesian metres east, north and up
// from `origin` (an ECEF point at the anchor's radial height). It holds only within the error budget
// of fn/LocalFrame.hpp; radial height stays authoritative.
struct LocalFrame {
    Vec3 origin;
    SurfaceFrame axes;
};

} // namespace derivean::sim
