#include "geometry/fn/Frame.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Projection.hpp"

#include <algorithm>
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

namespace {

// A unit axis perpendicular to d and to the coordinate axis d leans on least (ties to the lower index):
// it depends on |d| only, so d and −d give opposite axes.
Vec3 axisAcross(Vec3 d) {
    const double ax = std::abs(d.x), ay = std::abs(d.y), az = std::abs(d.z);
    const Vec3 least = ax <= ay && ax <= az ? Vec3{1, 0, 0} : ay <= az ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
    return normalized(cross(d, least));
}

} // namespace

// Up to a right angle, R v = v − ((a + b)·v / (1 + a·b)) (a + b) + 2 (a·v) b, the rotation about a × b
// taking a to b with no axis normalisation, so it stays exact for neighbouring cells a fraction of a
// metre apart (1 + a·b >= 1). Beyond it 1 + a·b goes to zero, so Rodrigues' form about the unit axis
// takes over: v cos θ + (n × v) sin θ + n (n·v)(1 − cos θ), with cos θ = a·b. The axis is taken as
// a × (a + b), equal to a × b: near the antipode a + b is a small vector of small absolute error, so
// the axis keeps full relative precision where a × b would cancel to an error of 1e-16 / sin θ.
// a + b is first scaled by an exact power of two to a largest component in [0.5, 1): otherwise its
// length would underflow below sin θ ≈ 1e-154, and subnormal components would round the cross
// product's terms to the absolute 1e-324 grid and tilt the axis (Codex DREV-CLAUDE-002-04).
Vec3 transport(Vec3 vector, Vec3 from, Vec3 to) {
    const Vec3 a = normalized(from), b = normalized(to);
    const double cosine = dot(a, b);
    if (cosine >= 0) {
        const Vec3 sum = a + b;
        return vector - sum * (dot(sum, vector) / (1 + cosine)) + b * (2 * dot(a, vector));
    }
    const Vec3 sum = a + b;
    int exponent = 0;
    std::frexp(std::max({std::abs(sum.x), std::abs(sum.y), std::abs(sum.z)}), &exponent);
    const Vec3 normal = cross(a, {std::ldexp(sum.x, -exponent), std::ldexp(sum.y, -exponent), std::ldexp(sum.z, -exponent)});
    const double scaled = length(normal);
    const double sine = std::ldexp(scaled, exponent);
    const Vec3 axis = scaled > 0 ? normal * (1 / scaled) : axisAcross(a - b);
    return vector * cosine + cross(axis, vector) * sine + axis * (dot(axis, vector) * (1 - cosine));
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
