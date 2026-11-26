#include "geometry/fn/SpherePolygon.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Rotation.hpp"

#include <algorithm>

namespace derivean::sim {

SpherePolygon polygonOf(const Cell& cell) {
    SpherePolygon polygon;
    polygon.count = 4;
    for (int k = 0; k < 4; ++k) polygon.corner[size_t(k)] = cornerOf(cell, k);
    return polygon;
}

SpherePolygon polygonOf(const SphereGrid& grid, int cell) {
    SpherePolygon polygon;
    polygon.count = 4;
    for (int k = 0; k < 4; ++k) polygon.corner[size_t(k)] = grid.vertex[size_t(grid.corners[size_t(cell)][size_t(k)])].point;
    return polygon;
}

SpherePolygon rotated(const SpherePolygon& polygon, Rotation rotation) {
    SpherePolygon result;
    result.count = polygon.count;
    const RotationMatrix matrix = matrixOf(rotation);
    for (int k = 0; k < polygon.count; ++k) result.corner[size_t(k)] = rotate(matrix, polygon.corner[size_t(k)]);
    return result;
}

PolygonCap capOf(const SpherePolygon& polygon) {
    Vec3 sum;
    for (int k = 0; k < polygon.count; ++k) sum = sum + polygon.corner[size_t(k)];
    PolygonCap cap;
    cap.centre = normalized(sum);
    for (int k = 0; k < polygon.count; ++k) cap.reach = std::max(cap.reach, length(polygon.corner[size_t(k)] - cap.centre));
    return cap;
}

} // namespace derivean::sim
