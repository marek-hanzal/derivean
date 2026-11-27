#pragma once

#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

namespace derivean::sim {

// The weather's tables for a grid of geometry/fn/SphereGrid (edge gradients, diamonds, vertex
// weights, upstream cells), in the grid's edge and vertex order.
SphereStencil makeSphereStencil(const SphereGrid& grid);

// Drops the edge tables only the Laplacian's assembly and the diagnostic operators read (82 MB at
// 6 × 256²); the step and the tracer keep working on the rest.
void releaseEdgeTables(SphereStencil& stencil);

} // namespace derivean::sim
