#pragma once

#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/type/SpherePolygon.hpp"

#include <array>

namespace derivean::sim {

// Areas of convex geodesic polygons and of their overlap, the weights of Jones's (1999) first-order
// conservative coupling, computed as CSLAM does on the cubed sphere (Lauritzen, Nair & Ullrich 2010):
// both polygons are projected gnomonically onto the plane tangent at the first one's centre, where
// great circles are straight lines; the first is clipped by each edge of the second (Sutherland &
// Hodgman 1974); the area is the sum of the spherical excesses of the clipped polygon's fan, from
// van Oosterom & Strackee's (1983) tan(E/2) with the corners' plane coordinates. That is the excess
// of the corners taken back onto the sphere, with the triple product as a plane cross product of
// corner differences, so it keeps its relative precision on small polygons.
//
// Inputs: corners counter-clockwise from outside, each polygon within about 20° of its centre (cells
// of level 2 and finer, and their rotations). The result is m² on the sphere of PlanetRadius, zero
// for polygons that are apart or meet only along an edge or at a corner (to rounding: about 1e-16 of
// a cell). A polygon with itself gives its own area. overlapArea(a, b) and overlapArea(b, a) agree to
// rounding.
//
// Clipping one footprint by the cells of a grid in turn uses the same tangent plane for every cell
// (the footprint's), so the pieces on two neighbouring cells are cut by the same projected line; over
// a footprint's candidates they sum to its area to about 4e-14 at level 7 (docs/handoff/geometry.md).
double areaOf(const SpherePolygon& polygon);
double overlapArea(const SpherePolygon& a, const SpherePolygon& b);

// A polygon made ready to overlap many others: its cap and its corners on the plane tangent at the
// cap's centre, as overlapArea(a, b) projects them; overlapArea(preparedOf(a), b) is overlapArea(a, b)
// bit for bit.
struct PreparedPolygon {
    PolygonCap cap;
    Vec3 first, second;   // the tangent plane's axes
    int count{};
    std::array<double, 8> x, y;
};
PreparedPolygon preparedOf(const SpherePolygon& polygon);
double overlapArea(const PreparedPolygon& a, const SpherePolygon& b);
// The same with b's cap, capOf(b), already taken.
double overlapArea(const PreparedPolygon& a, const SpherePolygon& b, const PolygonCap& capB);

} // namespace derivean::sim
