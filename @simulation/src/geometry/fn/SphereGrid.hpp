#pragma once

#include "geometry/type/Cell.hpp"
#include "geometry/type/SphereGrid.hpp"

namespace derivean::sim {

// Builds the tables of one level, reading neighbours, corners, areas, edge normals and lengths from
// the cell functions of this domain only. Levels 0 to 12 (6 × 4096² cells).
SphereGrid makeSphereGrid(int level);

inline int sphereCellCount(int level) { return 6 << (2 * level); }
inline int indexOf(const SphereGrid& grid, const Cell& cell) { return (cell.face * grid.side + cell.j) * grid.side + cell.i; }
inline Cell cellAt(const SphereGrid& grid, int index) {
    const int perFace = grid.side * grid.side, rest = index % perFace;
    return {index / perFace, grid.level, rest % grid.side, rest / grid.side};
}

// The cell across edge k of cell `cell`, by the grid's tables.
inline int neighbourOf(const SphereGrid& grid, int cell, int k) {
    const SphereEdge& edge = grid.edge[size_t(grid.edges[size_t(cell)][size_t(k)])];
    return edge.left == cell ? edge.right : edge.left;
}

// Where a cell sits for error budgets: inside a face, on a face edge, or touching a cube vertex.
enum class SpherePlace { Interior, FaceEdge, CubeCorner };
SpherePlace placeOf(const SphereGrid& grid, int cell);

} // namespace derivean::sim
