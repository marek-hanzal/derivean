#pragma once

#include "geometry/type/Cell.hpp"
#include "geometry/type/Rotation.hpp"
#include "geometry/type/SphereGrid.hpp"
#include "geometry/type/SpherePolygon.hpp"

namespace derivean::sim {

// A cell as a polygon: its corners 0..3 by cornerOf, or by the grid's vertices, whose points are
// shared bit for bit by every cell meeting there, so the polygons of a grid tile the sphere without
// slivers across face edges.
SpherePolygon polygonOf(const Cell& cell);
SpherePolygon polygonOf(const SphereGrid& grid, int cell);

// Every corner rotated; a rotation keeps the polygon convex and counter-clockwise.
SpherePolygon rotated(const SpherePolygon& polygon, Rotation rotation);

// The centre of the polygon's corners (their normalized sum) and the largest chord from it to a
// corner, on the unit sphere. A convex polygon smaller than a hemisphere lies inside the cap of that
// chord about that centre.
struct PolygonCap {
    Vec3 centre;
    double reach{};
};
PolygonCap capOf(const SpherePolygon& polygon);

} // namespace derivean::sim
