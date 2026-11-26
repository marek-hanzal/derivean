#pragma once

#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// A rotation of the sphere about its centre as a unit quaternion (Hamilton 1844): w = cos(θ/2) and
// (x, y, z) = sin(θ/2) · axis, the Euler–Rodrigues parameters of Rodrigues (1840). q and −q are the
// same rotation. The default is the identity.
struct Rotation {
    double w{1};
    double x{};
    double y{};
    double z{};
};

// The same rotation as an orthonormal 3 × 3 matrix by rows, for applying one rotation to many vectors.
struct RotationMatrix {
    Vec3 row[3]{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
};

} // namespace derivean::sim
