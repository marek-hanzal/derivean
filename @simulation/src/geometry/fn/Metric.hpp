#pragma once

#include "geometry/type/Cell.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// True metric of the sphere of PlanetRadius. Cell edges are great-circle arcs (lines of constant u or
// v on a face plane are planes through the centre), so a cell is a geodesic quadrilateral and its area
// is its spherical excess times R² (van Oosterom & Strackee 1983 for each half). Both are evaluated
// from the cell's gnomonic coordinates with the coordinate steps taken in closed form, which keeps them
// to about 1e-15 relative from a whole face down to MaxLevel; ledgers use these areas, never a
// constant.
double areaOf(const Cell& cell);                 // m²
double edgeLengthOf(const Cell& cell, int edge); // m, along the great circle

// The angle between two vectors seen from the centre (any lengths) and the great-circle distance on
// the datum sphere between the directions they point to; both stay accurate for tiny and for nearly
// antipodal separations.
double centralAngle(Vec3 a, Vec3 b);
double distanceBetween(Vec3 a, Vec3 b);          // m

// The spherical excess of the geodesic triangle a, b, c (directions of any length), in steradians.
double sphericalExcess(Vec3 a, Vec3 b, Vec3 c);

} // namespace derivean::sim
