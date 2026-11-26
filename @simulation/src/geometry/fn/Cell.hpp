#pragma once

#include "geometry/type/Cell.hpp"
#include "geometry/type/Vec3.hpp"

#include <cstdint>

namespace derivean::sim {

inline int cellsPerSide(int level) { return 1 << level; }
bool isValid(const Cell& cell);

// The cell holding a point: a face point, or any non-zero ECEF vector. A point on a face edge belongs
// to the face facePointOf picks. Inside a face a point exactly on a cell edge in ST goes to the cell
// above it in i or j, but through UV rounding a point within an ulp of an edge may land on either side:
// corners are not mapped back to cells.
Cell cellOf(FacePoint point, int level);
Cell cellOf(Vec3 direction, int level);

// Unit directions of the cell's centre (the centre of its ST square) and of its corners (numbered as in
// type/Cell.hpp).
Vec3 centreOf(const Cell& cell);
Vec3 cornerOf(const Cell& cell, int corner);
FacePoint centrePointOf(const Cell& cell);

// The quadtree: the ancestor at a level no finer than the cell's, and child k (bit 0 picks the upper
// half in i, bit 1 in j) of a cell coarser than MaxLevel.
Cell parentOf(const Cell& cell, int level);
Cell childOf(const Cell& cell, int k);

// The stable 64-bit key of GeometryVersion (layout in constant/Sphere.hpp). Ordered face first, then
// in Z order; the keys of a cell's descendants lie strictly within key ± its lowest set bit. Key 0 is
// no cell.
std::uint64_t keyOf(const Cell& cell);
Cell cellOfKey(std::uint64_t key);

} // namespace derivean::sim
