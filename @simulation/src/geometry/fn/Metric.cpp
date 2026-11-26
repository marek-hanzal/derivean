#include "geometry/fn/Metric.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Projection.hpp"

#include <cmath>
#include <numbers>

namespace derivean::sim {

namespace {

// A cell's gnomonic bounds and their differences. du = tan b − tan a = sin(b − a) / (cos a cos b),
// with b − a = (π/2) / 2^level exact, so the step keeps full relative precision even where u0 and u1
// agree in nearly every digit.
struct Bounds {
    double u0, u1, v0, v1, du, dv;
};

double angleAt(int index, int level) { return std::numbers::pi / 4 * (2.0 * index / cellsPerSide(level) - 1); }

double stepOf(int index, int level) {
    const double a = angleAt(index, level), b = angleAt(index + 1, level);
    return std::sin(std::numbers::pi / 2 / cellsPerSide(level)) / (std::cos(a) * std::cos(b));
}

Bounds boundsOf(const Cell& cell) {
    const double count = cellsPerSide(cell.level);
    return {stToUv(cell.i / count), stToUv((cell.i + 1) / count), stToUv(cell.j / count), stToUv((cell.j + 1) / count),
            stepOf(cell.i, cell.level), stepOf(cell.j, cell.level)};
}

// Excess of one of the two triangles a cell splits into, tan(E/2) = |a·(b×c)| / (1 + a·b + b·c + c·a)
// for unit corners (van Oosterom & Strackee 1983). On the face plane the corners are (1, u, v); the
// triple product of the triangle's unnormalised corners is du · dv for both halves.
double halfExcess(double triple, Vec3 a, Vec3 b, Vec3 c) {
    const double na = length(a), nb = length(b), nc = length(c);
    const double denominator = 1 + dot(a, b) / (na * nb) + dot(b, c) / (nb * nc) + dot(c, a) / (nc * na);
    return 2 * std::atan2(triple / (na * nb * nc), denominator);
}

} // namespace

double areaOf(const Cell& cell) {
    const Bounds b = boundsOf(cell);
    const Vec3 p00{1, b.u0, b.v0}, p10{1, b.u1, b.v0}, p11{1, b.u1, b.v1}, p01{1, b.u0, b.v1};
    const double triple = b.du * b.dv;
    return (halfExcess(triple, p00, p10, p11) + halfExcess(triple, p00, p11, p01)) * PlanetRadius * PlanetRadius;
}

// For corners (1, u, v) and (1, u', v) the cross product has length du·sqrt(1 + v²) and the dot
// product is 1 + u·u' + v²; likewise along v.
double edgeLengthOf(const Cell& cell, int edge) {
    const Bounds b = boundsOf(cell);
    double angle = 0;
    if (edge % 2 == 0) {
        const double v = edge == 0 ? b.v0 : b.v1;
        angle = std::atan2(b.du * std::sqrt(1 + v * v), 1 + b.u0 * b.u1 + v * v);
    } else {
        const double u = edge == 1 ? b.u1 : b.u0;
        angle = std::atan2(b.dv * std::sqrt(1 + u * u), 1 + u * u + b.v0 * b.v1);
    }
    return angle * PlanetRadius;
}

double centralAngle(Vec3 a, Vec3 b) { return std::atan2(length(cross(a, b)), dot(a, b)); }

double distanceBetween(Vec3 a, Vec3 b) { return centralAngle(a, b) * PlanetRadius; }

double sphericalExcess(Vec3 a, Vec3 b, Vec3 c) {
    const Vec3 ua = normalized(a), ub = normalized(b), uc = normalized(c);
    const double triple = dot(ua, cross(ub - ua, uc - ua));
    return 2 * std::atan2(std::abs(triple), 1 + dot(ua, ub) + dot(ub, uc) + dot(uc, ua));
}

} // namespace derivean::sim
