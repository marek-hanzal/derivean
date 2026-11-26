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

} // namespace derivean::sim
