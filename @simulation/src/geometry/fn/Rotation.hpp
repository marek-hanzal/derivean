#pragma once

#include "geometry/type/Rotation.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// Rotations are pure functions of their arguments with no state, so the same calls in the same order
// give the same bits on every thread.
//
// The rotation by `angle` (rad, right-handed) about `axis` (any non-zero length).
Rotation rotationAbout(Vec3 axis, double angle);

// The rotation an Euler vector ω (rad/s: axis times rate, right-handed, as a plate's Euler pole)
// makes in `time` (s): angle |ω| · time about ω. The vector part is ω · time/2 · sinc(|ω| time / 2),
// never ω / |ω|, so it stays exact to first order down to any small angle (ω · dt of 1e-9 rad and
// below keeps its full relative precision) and ω = 0 gives the identity bit for bit.
Rotation rotationOf(Vec3 euler, double time);

// `later` after `first` (the Hamilton product later · first): rotating by the result is rotating by
// `first`, then by `later`. Stepping a piece by a plate's rotation over a step is
// compose(rotationOf(ω, dt), Q). The product of unit quaternions drifts from unit length by rounding
// (about 1e-16 per product); renormalize after each step.
Rotation compose(Rotation later, Rotation first);
Rotation inverse(Rotation rotation);        // the conjugate: exact for a unit quaternion
Rotation renormalized(Rotation rotation);   // back to unit length; the zero quaternion stays zero

// The angle of the rotation in [0, π], 2 atan2(|(x, y, z)|, |w|): accurate for tiny angles too.
double angleOf(Rotation rotation);

// v rotated: v + w t + (x, y, z) × t with t = 2 (x, y, z) × v.
Vec3 rotate(Rotation rotation, Vec3 v);

RotationMatrix matrixOf(Rotation rotation);
Vec3 rotate(const RotationMatrix& matrix, Vec3 v);

} // namespace derivean::sim
