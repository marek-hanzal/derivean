#pragma once

#include "geometry/type/Vec3.hpp"

#include <array>
#include <vector>

namespace derivean::sim {

// The weather's finite-volume discretisation on top of the geometry's sphere grid
// (geometry/type/SphereGrid.hpp): per edge and per vertex, in the grid's edge and vertex order.
//
// The gradient at an edge's midpoint comes from two differences: across the edge (right − left) and
// along it (to − from, values at the vertices). In the gnomonic tangent plane at the midpoint `across`
// and `along` are the dual basis of the two chords, so gradient = Δacross · across + Δalong · along is
// exact for a field linear in that plane. Because the chord between the centres is not the normal
// (Putman & Lin 2007), `across` is not along the normal: this is the non-orthogonal metric term,
// carried by one formula on every edge, face seams and cube vertices included.
struct EdgeStencil {
    double diamond{};   // m², the two geodesic triangles (left centre, from, to) and (right centre, to, from)
    Vec3 across;        // 1/m
    Vec3 along;         // 1/m
    Vec3 chord;         // m, to − from in the tangent plane: the edge's share of a circulation
    // From the middle of the chord between the two centres to the edge's midpoint, in the tangent
    // plane. Inside a face it is O(h²); on a face edge it is O(h) along the edge, because the two
    // faces' grid lines meet the edge obliquely, so a value carried through the edge is the mean of
    // its two cells plus gradient · offset.
    Vec3 offset;        // m
};

// `weight[v]` gives a value at vertex v from the values of its cells (SphereVertex::cells, in that
// order). The weights are the least-squares fit of a linear field in the gnomonic tangent plane at
// the vertex, so they sum to one and reproduce linear fields exactly. A plain mean would not: on a
// face edge the grid lines of the two faces meet the edge obliquely (60° at a cube vertex), so the
// four centres' mean sits O(h) from the vertex along the edge (a quarter of a cell next to a cube
// vertex).
//
// `carry[e]` is edge e's offset projected on its gradient weights, (offset · across, offset · along):
// the advected value's move to the edge's midpoint is carry · (right − left, to − from), read in
// 16 bytes an edge instead of the three vectors' 72.
//
// A step reads only the hot tables: carry, weight, `vertexCells` and `behind`. `edge` (104 bytes an
// edge) builds the Laplacian and serves the diagnostic operators (gradientOf, curlOf), and
// releaseEdgeTables drops it once the Poisson solver is made.
struct SphereStencil {
    std::vector<EdgeStencil> edge;
    std::vector<std::array<double, 4>> weight;
    std::vector<std::array<double, 2>> carry;
    // Per vertex its cells (SphereVertex::cells), four always: a cube vertex's three are followed by
    // its first again, whose weight is zero, so a vertex value is one four-term sum without the grid's
    // 48-byte vertex.
    std::vector<std::array<int, 4>> vertexCells;
    // Per edge the cells behind left and behind right: across the edge opposite this one in each
    // cell's own numbering, the upstream cell of a limited advective flux.
    std::vector<std::array<int, 2>> behind;
};

} // namespace derivean::sim
