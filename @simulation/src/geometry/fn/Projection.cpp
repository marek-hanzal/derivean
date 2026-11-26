#include "geometry/fn/Projection.hpp"

#include <cmath>
#include <numbers>

namespace derivean::sim {

// stToUv is exactly odd about the face centre on cell boundaries and exact at the face edges (where S2's
// tangent projection adds a 2^-53 correction instead), so the grids of two faces meet edge to edge on
// the shared cube edge.
double stToUv(double s) {
    const double t = 2 * s - 1;
    if (t >= 1) return 1;
    if (t <= -1) return -1;
    return std::tan(std::numbers::pi / 4 * t);
}

double uvToSt(double u) {
    if (u >= 1) return 1;
    if (u <= -1) return 0;
    return 0.5 * (1 + std::atan(u) * (4 / std::numbers::pi));
}

Vec3 faceUvToCube(int face, double u, double v) {
    switch (face) {
        case 0: return {1, u, v};
        case 1: return {-u, 1, v};
        case 2: return {-u, -v, 1};
        case 3: return {-1, -v, -u};
        case 4: return {v, -1, -u};
        default: return {v, u, -1};
    }
}

FacePoint facePointOf(Vec3 p) {
    const double ax = std::abs(p.x), ay = std::abs(p.y), az = std::abs(p.z);
    const int axis = ax > ay ? (ax > az ? 0 : 2) : (ay > az ? 1 : 2);
    const double along = axis == 0 ? p.x : axis == 1 ? p.y : p.z;
    const int face = along < 0 ? axis + 3 : axis;
    switch (face) {
        case 0: return {0, p.y / p.x, p.z / p.x};
        case 1: return {1, -p.x / p.y, p.z / p.y};
        case 2: return {2, -p.x / p.z, -p.y / p.z};
        case 3: return {3, p.z / p.x, p.y / p.x};
        case 4: return {4, p.z / p.y, -p.x / p.y};
        default: return {5, -p.y / p.z, -p.x / p.z};
    }
}

Vec3 directionOf(FacePoint point) { return normalized(faceUvToCube(point.face, point.u, point.v)); }

Vec3 faceAxisU(int face) { return faceUvToCube(face, 1, 0) - faceUvToCube(face, 0, 0); }
Vec3 faceAxisV(int face) { return faceUvToCube(face, 0, 1) - faceUvToCube(face, 0, 0); }
Vec3 faceNormal(int face) { return faceUvToCube(face, 0, 0); }

} // namespace derivean::sim
