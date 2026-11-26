#pragma once

#include "geometry/type/Vec3.hpp"

#include <array>
#include <vector>

namespace derivean::sim {

// The weather grid on the sphere: every cell of one level of S1's cubed sphere (geometry/**), with
// the tables a finite-volume solver reads every step. The level is a parameter (6 × 128² cells at
// level 7, 6 × 256² at level 8); nothing here assumes one. Cells are numbered
// (face · side + j) · side + i, edges and vertices in the order makeSphereGrid meets them.
//
// One edge per pair of neighbouring cells, so a flux through it is evaluated once and applied to its
// two cells with opposite signs. `left` is the cell whose outward normal the edge carries (the cell
// with the lower index); `from` and `to` are its corners in left's counter-clockwise order, so the
// edge runs from → to round left and to → from round right.
struct SphereEdge {
    int left{};
    int right{};
    int from{};         // vertex
    int to{};           // vertex
    double length{};    // m, S1's edgeOf
    double diamond{};   // m², the two geodesic triangles (left centre, from, to) and (right centre, to, from)
    Vec3 normal;        // unit, out of left, tangent along the whole edge (S1's edgeOf)
    // The gradient at the edge's midpoint from two differences: across the edge (right − left) and
    // along it (to − from, values at the vertices). In the gnomonic tangent plane at the midpoint
    // `across` and `along` are the dual basis of the two chords, so gradient = Δacross · across +
    // Δalong · along is exact for a field linear in that plane. Because the chord between the centres
    // is not the normal (Putman & Lin 2007), `across` is not along the normal: this is the
    // non-orthogonal metric term, carried by one formula on every edge, face seams and cube vertices
    // included.
    Vec3 across;        // 1/m
    Vec3 along;         // 1/m
    Vec3 chord;         // m, to − from in the tangent plane: the edge's share of a circulation
    // From the middle of the chord between the two centres to the edge's midpoint, in the tangent
    // plane. Inside a face it is O(h²); on a face edge it is O(h) along the edge, because the two
    // faces' grid lines meet the edge obliquely, so a value carried through the edge is the mean of
    // its two cells plus gradient · offset.
    Vec3 offset;        // m
    // The cells behind left and behind right: across the edge opposite this one in each cell's own
    // numbering, the upstream cell of a limited advective flux.
    int behindLeft{};
    int behindRight{};
};

// A vertex of the grid with the cells that meet there: four, or three at a cube vertex, and the
// weights that give a value at the vertex from theirs. The weights are the least-squares fit of a
// linear field in the gnomonic tangent plane at the vertex, so they sum to one and reproduce linear
// fields exactly. A plain mean would not: on a face edge the grid lines of the two faces meet the
// edge obliquely (60° at a cube vertex), so the four centres' mean sits O(h) from the vertex along
// the edge (a quarter of a cell next to a cube vertex).
struct SphereVertex {
    int count{};
    std::array<int, 4> cells{};
    std::array<double, 4> weight{};
};

struct SphereGrid {
    int level{};
    int side{};                         // cells along a face edge
    int cells{};                        // 6 · side²
    std::vector<double> area;           // m², S1's areaOf
    std::vector<Vec3> centre;           // unit direction
    std::vector<double> latitudeSine;   // sin φ of the centre
    std::vector<std::array<int, 4>> edges;     // per cell, in the cell's edge order
    std::vector<std::array<int, 4>> corners;   // per cell, vertex per corner
    std::vector<SphereEdge> edge;
    std::vector<SphereVertex> vertex;
};

// A sparse symmetric matrix in rows (compressed sparse rows), the cells' Laplacian stiffness.
struct SphereMatrix {
    std::vector<int> start;       // row r holds entries start[r] .. start[r + 1] − 1
    std::vector<int> column;
    std::vector<double> value;
    std::vector<double> diagonal;
};

} // namespace derivean::sim
