#pragma once

#include "geometry/type/Cell.hpp"
#include "geometry/type/Frame.hpp"

namespace derivean::sim {

// Positions: ECEF metres as doubles are the identity of a place, with radial height above the datum
// sphere authoritative. Directions are ECEF vectors of length 1e-15 to 1e150; `normalized` makes one
// unit (shorter ones are not scaled up, longer ones overflow its squared length).
Vec3 ecefOf(Vec3 direction, double height);   // direction of any length
double heightOf(Vec3 ecef);                   // m above PlanetRadius, along the radius

LatLon latLonOf(Vec3 direction);
Vec3 directionFromLatLon(LatLon place);

// Local up, north and east (first = east, second = north). On a pole east is taken as at longitude 0,
// (0, 1, 0), so the frame is defined everywhere.
SurfaceFrame geographicFrame(Vec3 direction);

// The grid frame of a face at a point: first along the face's +u grid line. The frame of a cell is
// the one at its centre; solvers keep vector components in it.
SurfaceFrame faceFrame(FacePoint point);
SurfaceFrame cellFrame(const Cell& cell);

Vec3 vectorOf(TangentVector tangent, const SurfaceFrame& frame);
TangentVector tangentOf(Vec3 vector, const SurfaceFrame& frame);  // drops the component along up

// The same vector in another frame at the same point (both frames share `up`).
TangentVector reframe(TangentVector tangent, const SurfaceFrame& from, const SurfaceFrame& to);

// Parallel transport of a tangent vector along the shorter great circle from one direction to another:
// the rotation about from × to that takes from onto to (Rodrigues 1840). It is finite and reversible
// (transport back returns the vector) to about 1e-14 for every pair, up to and including antipodes.
// Only at the exact antipode (the unit directions sum to zero) is the great circle undefined; the axis
// is then chosen deterministically, perpendicular to from − to and to the coordinate axis that vector
// leans on least. Round a closed geodesic polygon the result turns by the polygon's enclosed solid
// angle in the sense of travel, the holonomy of the sphere (Gauss–Bonnet; do Carmo 1976).
Vec3 transport(Vec3 vector, Vec3 from, Vec3 to);

// A vector held in one cell's frame, carried to another cell (across an edge or a corner, also onto
// another face) along the geodesic between their centres and given in that cell's frame. Carrying it
// back returns the original components. The turn is fixed per pair of cells, so a solver can keep its
// FrameRotation per edge and apply it with `turned`. Antipodal cells follow transport's rule.
FrameRotation carryRotation(const Cell& from, const Cell& to);
TangentVector turned(TangentVector tangent, FrameRotation rotation);
TangentVector carry(TangentVector tangent, const Cell& from, const Cell& to);

} // namespace derivean::sim
