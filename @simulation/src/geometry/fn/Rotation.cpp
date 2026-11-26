#include "geometry/fn/Rotation.hpp"

#include <cmath>

namespace derivean::sim {

Rotation rotationAbout(Vec3 axis, double angle) {
    const Vec3 unit = normalized(axis);
    const double half = 0.5 * angle, sine = std::sin(half);
    return {std::cos(half), unit.x * sine, unit.y * sine, unit.z * sine};
}

Rotation rotationOf(Vec3 euler, double time) {
    const double rate = length(euler);
    const double half = 0.5 * rate * time;
    // sin(half) / half: std::sin keeps full relative precision for any small half, so the quotient
    // does too; only half = 0 needs the limit.
    const double sinc = half == 0 ? 1.0 : std::sin(half) / half;
    const double scale = 0.5 * time * sinc;
    return {std::cos(half), euler.x * scale, euler.y * scale, euler.z * scale};
}

Rotation compose(Rotation a, Rotation b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}

Rotation inverse(Rotation q) { return {q.w, -q.x, -q.y, -q.z}; }

Rotation renormalized(Rotation q) {
    const double norm = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    if (norm == 0) return q;
    const double scale = 1 / norm;
    return {q.w * scale, q.x * scale, q.y * scale, q.z * scale};
}

double angleOf(Rotation q) { return 2 * std::atan2(std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z), std::abs(q.w)); }

Vec3 rotate(Rotation q, Vec3 v) {
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = cross(u, v) * 2.0;
    return v + t * q.w + cross(u, t);
}

RotationMatrix matrixOf(Rotation q) {
    const double xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const double xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z, wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    RotationMatrix m;
    m.row[0] = {1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy)};
    m.row[1] = {2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx)};
    m.row[2] = {2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy)};
    return m;
}

Vec3 rotate(const RotationMatrix& m, Vec3 v) { return {dot(m.row[0], v), dot(m.row[1], v), dot(m.row[2], v)}; }

} // namespace derivean::sim
