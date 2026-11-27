#pragma once

#include "geometry/type/Vec3.hpp"

#include <array>

namespace derivean::sim {

// A convex geodesic polygon on the sphere: `count` corners (unit directions) counter-clockwise seen
// from outside, joined by great-circle arcs. A cell is one (four corners), and so is a crust piece's
// footprint, its birth cell rotated. Up to eight corners: the overlap of two quadrilaterals has at
// most eight.
struct SpherePolygon {
    int count{};
    std::array<Vec3, 8> corner{};
};

// The centre of a polygon's corners (their normalized sum) and the largest chord from it to a corner,
// on the unit sphere (fn/SpherePolygon's capOf). A convex polygon smaller than a hemisphere lies inside
// the cap of that chord about that centre.
struct PolygonCap {
    Vec3 centre;
    double reach{};
};

} // namespace derivean::sim
