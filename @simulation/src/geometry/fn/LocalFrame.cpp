#include "geometry/fn/LocalFrame.hpp"

#include "geometry/fn/Frame.hpp"

#include <cmath>

namespace derivean::sim {

LocalFrame anchorAt(Vec3 ecef) { return {ecef, geographicFrame(ecef)}; }

Vec3 localOf(const LocalFrame& frame, Vec3 ecef) {
    const Vec3 offset = ecef - frame.origin;
    return {dot(offset, frame.axes.first), dot(offset, frame.axes.second), dot(offset, frame.axes.up)};
}

Vec3 ecefOfLocal(const LocalFrame& frame, Vec3 local) {
    return frame.origin + frame.axes.first * local.x + frame.axes.second * local.y + frame.axes.up * local.z;
}

// The height uses d² / (sqrt(r² + d²) + r) and the distance the series of x − atan x below x = 0.01
// (relative error under x⁶/3 ≈ 3e-13), so neither cancels for small d.
TangentError tangentErrorAt(double distance, double radius) {
    const double d = std::abs(distance);
    const double x = d / radius;
    const double angle = std::atan(x);
    const double x2 = x * x;
    const double excess = x < 0.01 ? x * x2 * (1.0 / 3 - x2 * (1.0 / 5 - x2 / 7)) : x - angle;
    return {d * d / (std::hypot(radius, d) + radius), angle, radius * excess};
}

double tangentReach(double tolerance, double radius) { return std::sqrt(tolerance * (2 * radius + tolerance)); }

} // namespace derivean::sim
