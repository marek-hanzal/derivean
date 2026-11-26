#pragma once

#include "geometry/type/Cell.hpp"

namespace derivean::sim {

// The one owner of adjacency on the sphere: processes read neighbours here and never step i or j by
// hand, because across a face edge the neighbour lies on another face with its own axes.
Neighbour neighbourAcross(const Cell& cell, int edge);
EdgeNeighbours neighboursOf(const Cell& cell);   // in edge order

// Where an edge of a face leads (derived from S2's face mapping, checked by the tests against the
// shared corners).
FaceEdge faceEdgeOf(int face, int edge);

CornerCells cellsAround(const Cell& cell, int corner);

// True when the corner is a cube vertex, where three cells meet instead of four.
bool atCubeVertex(const Cell& cell, int corner);

} // namespace derivean::sim
