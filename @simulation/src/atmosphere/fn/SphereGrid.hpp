#pragma once

#include "atmosphere/type/SphereGrid.hpp"
#include "geometry/type/Cell.hpp"

namespace derivean::sim {

// Builds every table of the weather grid at one level of S1's cubed sphere, reading neighbours,
// corners, areas, edge normals and lengths from geometry/** only.
SphereGrid makeSphereGrid(int level);

inline int indexOf(const SphereGrid& grid, const Cell& cell) { return (cell.face * grid.side + cell.j) * grid.side + cell.i; }
inline Cell cellAt(const SphereGrid& grid, int index) {
    const int perFace = grid.side * grid.side, rest = index % perFace;
    return {index / perFace, grid.level, rest % grid.side, rest / grid.side};
}

// Where a cell sits for error budgets: inside a face, on a face edge, or touching a cube vertex.
enum class SpherePlace { Interior, FaceEdge, CubeCorner };
SpherePlace placeOf(const SphereGrid& grid, int cell);

} // namespace derivean::sim
