#include "geometry/fn/Frame.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Projection.hpp"

#include <cmath>

namespace derivean::sim {

Vec3 ecefOf(Vec3 direction, double height) { return normalized(direction) * (PlanetRadius + height); }

double heightOf(Vec3 ecef) { return length(ecef) - PlanetRadius; }

LatLon latLonOf(Vec3 d) { return {std::atan2(d.z, std::hypot(d.x, d.y)), std::atan2(d.y, d.x)}; }

Vec3 directionFromLatLon(LatLon place) {
    const double c = std::cos(place.latitude);
    return {c * std::cos(place.longitude), c * std::sin(place.longitude), std::sin(place.latitude)};
}

SurfaceFrame geographicFrame(Vec3 direction) {
    const Vec3 up = normalized(direction);
    const Vec3 toward = cross(Vec3{0, 0, 1}, up);
    const double size = length(toward);
    const Vec3 east = size > 1e-15 ? toward * (1 / size) : Vec3{0, 1, 0};
    return {east, cross(up, east), up};
}

SurfaceFrame faceFrame(FacePoint point) {
    const Vec3 up = directionOf(point);
    const Vec3 axis = faceAxisU(point.face);
    const Vec3 first = normalized(axis - up * dot(axis, up));
    return {first, cross(up, first), up};
}

SurfaceFrame cellFrame(const Cell& cell) { return faceFrame(centrePointOf(cell)); }

Vec3 vectorOf(TangentVector t, const SurfaceFrame& frame) { return frame.first * t.first + frame.second * t.second; }

TangentVector tangentOf(Vec3 vector, const SurfaceFrame& frame) { return {dot(vector, frame.first), dot(vector, frame.second)}; }

TangentVector reframe(TangentVector tangent, const SurfaceFrame& from, const SurfaceFrame& to) {
    return tangentOf(vectorOf(tangent, from), to);
}

// R v = v − ((a + b)·v / (1 + a·b)) (a + b) + 2 (a·v) b is the rotation about a × b taking a to b; it
// needs no axis normalisation, so it stays exact for neighbouring cells a fraction of a metre apart.
Vec3 transport(Vec3 vector, Vec3 from, Vec3 to) {
    const Vec3 a = normalized(from), b = normalized(to);
    const Vec3 sum = a + b;
    return vector - sum * (dot(sum, vector) / (1 + dot(a, b))) + b * (2 * dot(a, vector));
}

// Transport preserves orientation and both frames are right-handed, so the carried first axis fixes the
// whole turn; it is renormalised against rounding.
FrameRotation carryRotation(const Cell& from, const Cell& to) {
    const TangentVector axis = tangentOf(transport(cellFrame(from).first, centreOf(from), centreOf(to)), cellFrame(to));
    const double size = std::hypot(axis.first, axis.second);
    return {axis.first / size, axis.second / size};
}

TangentVector turned(TangentVector t, FrameRotation r) {
    return {r.cosine * t.first - r.sine * t.second, r.sine * t.first + r.cosine * t.second};
}

TangentVector carry(TangentVector tangent, const Cell& from, const Cell& to) { return turned(tangent, carryRotation(from, to)); }

} // namespace derivean::sim
