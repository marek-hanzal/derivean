#pragma once

#include "geometry/type/Cell.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// The face mapping of GeometryVersion 1. A face's ST coordinates in [0, 1] map to its gnomonic UV in
// [-1, 1] by the equiangular projection of the cubed sphere (Ronchi, Iacono & Paolucci 1996), so equal
// steps of s are equal angles seen from the planet's centre. Against S2's quadratic mapping it keeps the
// smallest cell at 0.83 of the mean area instead of 0.60 and the shortest edge 18 % longer; see
// docs/handoff/geometry.md.
double stToUv(double s);
double uvToSt(double u);

// S2's face numbering (Google S2 Geometry, s2coords.h, Veach 2005): faces 0..5 face +X, +Y, +Z, -X,
// -Y, -Z, with S2's u and v axes on each.
// `faceUvToCube` is the point on the face's cube plane (not unit length).
Vec3 faceUvToCube(int face, double u, double v);
FacePoint facePointOf(Vec3 direction);   // any non-zero vector; ties between faces go to the higher axis, as in S2
Vec3 directionOf(FacePoint point);       // unit length

// Unit vectors of a face: d(cube point)/du, d(cube point)/dv and the outward normal.
Vec3 faceAxisU(int face);
Vec3 faceAxisV(int face);
Vec3 faceNormal(int face);

} // namespace derivean::sim
