#pragma once

#include "geometry/type/SpherePolygon.hpp"
#include "geometry/type/Vec3.hpp"

#include <array>
#include <vector>

namespace derivean::sim {

// Every cell of one level of the cubed sphere as tables a process walks every step: the weather (S4a)
// and the planet's history (S3) read the same grid. The level is a parameter (6 × 128² cells at level
// 7, 6 × 256² at level 8); nothing here assumes one. Cells are numbered (face · side + j) · side + i
// (fn/SphereGrid.hpp: indexOf, cellAt), edges and vertices in the order makeSphereGrid meets them.
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
    double length{};    // m, fn/Edge's edgeOf from left
    Vec3 normal;        // unit, out of left, tangent along the whole edge (fn/Edge's edgeOf)
};

// A vertex of the grid with the cells that meet there: four, or three at a cube vertex. cells[0] is
// the cell that numbered the vertex, the others follow clockwise round it (fn/Neighbour's
// cellsAround); `point` is the corner as that first cell gives it (cornerOf). Seen from another of
// the cells the same corner may differ in the last bit across a face edge.
struct SphereVertex {
    int count{};
    std::array<int, 4> cells{};
    Vec3 point;         // unit
};

struct SphereGrid {
    int level{};
    int side{};                                // cells along a face edge
    int cells{};                               // 6 · side²
    std::vector<double> area;                  // m², fn/Metric's areaOf
    std::vector<Vec3> centre;                  // unit direction (cell centre of fn/Cell)
    std::vector<double> reach;                 // per cell, the largest chord from the centre to a corner (unit sphere)
    std::vector<PolygonCap> cap;               // per cell, capOf(polygonOf(grid, cell)) (fn/SpherePolygon)
    std::vector<std::array<int, 4>> edges;     // per cell, in the cell's edge order
    std::vector<std::array<int, 4>> corners;   // per cell, vertex per corner
    std::vector<SphereEdge> edge;
    std::vector<SphereVertex> vertex;
};

} // namespace derivean::sim
