#include "geometry/fn/Edge.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"
#include "geometry/fn/Projection.hpp"

namespace derivean::sim {

namespace {

// A vector given in a face's own right-handed basis (outward normal N, axes U and V). Each basis
// vector has a single non-zero component of ±1, so the conversion is exact.
Vec3 inFace(int face, double n, double u, double v) { return faceNormal(face) * n + faceAxisU(face) * u + faceAxisV(face) * v; }

} // namespace

// An edge lies on a plane of constant u or v through the centre. With cube points (1, u, v) in the
// face basis, the outward normal of edge 0 (v = v0) is (v0, 0, −1), of edge 1 (u = u1) (−u1, 1, 0),
// of edge 2 (v = v1) (−v1, 0, 1), of edge 3 (u = u0) (u0, −1, 0): exact in the edge's own coordinate,
// where a cross product of two corners a fraction of a metre apart would lose about 1e-16 / angle.
// The neighbour sees the same coordinate (on a face edge ±1 on both faces), so its normal is exactly
// the negation of ours, and the two views cancel exactly in a sum of fluxes.
CellEdge edgeOf(const Cell& cell, int edge) {
    const double count = cellsPerSide(cell.level);
    const double u = stToUv((cell.i + (edge == 1)) / count), v = stToUv((cell.j + (edge == 2)) / count);
    Vec3 normal;
    switch (edge) {
        case 0: normal = inFace(cell.face, v, 0, -1); break;
        case 1: normal = inFace(cell.face, -u, 1, 0); break;
        case 2: normal = inFace(cell.face, -v, 0, 1); break;
        default: normal = inFace(cell.face, u, -1, 0); break;
    }
    // On a reversed face seam the two views' closed-form lengths differ in the last bit, so the length
    // is always taken from the view of the cell with the lower key.
    const Neighbour next = neighbourAcross(cell, edge);
    const double length = keyOf(cell) < keyOf(next.cell) ? edgeLengthOf(cell, edge) : edgeLengthOf(next.cell, next.edge);
    const Vec3 from = cornerOf(cell, edge), to = cornerOf(cell, (edge + 1) % 4);
    return {from, to, normalized(from + to), normalized(normal), length};
}

} // namespace derivean::sim
