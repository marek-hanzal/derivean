#pragma once

#include "geometry/type/SpherePolygon.hpp"

#include <array>
#include <vector>

namespace derivean::sim {

// Coverage by a union of footprints, not a sum of overlaps (Codex V2, S3-design.md §4): where two
// footprints share the same 30 % of a third, the sum says 60 % and the union 30 %.
//
// Everything happens in one gnomonic tangent plane, where great circles are straight lines: convex
// geodesic polygons project to convex plane polygons, a clip is Sutherland & Hodgman's (1974) and the
// union of several is Clipper2's (Angus Johnson, 2010–2025, BSL-1.0) in integers at 10¹² per plane unit
// (about a radian near the tangent point; a corner error of 10⁻¹² on a cell of 0.011 rad is about
// 4 · 10⁻¹⁰ of its area). The union comes back as a tree of contours with holes (PolyTree64); each
// contour goes back onto the sphere and its signed area is the sum of the signed spherical excesses of
// the fan from the tangent point (van Oosterom & Strackee 1983), so an outer contour counts positive
// and a hole negative, which is checked contour by contour.

// The tangent plane at a unit direction: first × second = centre.
struct TangentPlane {
    Vec3 centre, first, second;
};

// A convex polygon in a plane's gnomonic coordinates, counter-clockwise seen from outside the sphere.
struct PlanePolygon {
    int count{};
    std::array<double, 16> x{}, y{};
};

TangentPlane tangentPlaneAt(Vec3 centre);
PlanePolygon projected(const TangentPlane& plane, const SpherePolygon& polygon);

// The part of `subject` inside the convex polygon `clip`.
PlanePolygon clipped(PlanePolygon subject, const PlanePolygon& clip);

// m² on the sphere of PlanetRadius of a convex plane polygon (its corners back on the sphere).
double sphericalArea(const PlanePolygon& polygon);

// The area of the union, m², and the contours it found.
struct UnionArea {
    double area{};
    int outers{}, holes{};
};
UnionArea unionArea(const std::vector<PlanePolygon>& polygons);

// |a ∩ (∪ others)| as |a| + |∪ others| − |a ∪ others|: how much of `a` the others cover together.
double coveredBy(const PlanePolygon& a, const std::vector<PlanePolygon>& others);

} // namespace derivean::sim
