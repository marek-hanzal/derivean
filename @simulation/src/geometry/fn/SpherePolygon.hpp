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

// The polygon's cap (type/SpherePolygon's PolygonCap).
PolygonCap capOf(const SpherePolygon& polygon);

} // namespace derivean::sim
