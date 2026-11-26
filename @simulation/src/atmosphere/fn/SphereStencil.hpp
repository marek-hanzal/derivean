#pragma once

#include "atmosphere/type/SphereStencil.hpp"
#include "geometry/type/SphereGrid.hpp"

namespace derivean::sim {

// The weather's tables for a grid of geometry/fn/SphereGrid (edge gradients, diamonds, vertex
// weights, upstream cells), in the grid's edge and vertex order.
SphereStencil makeSphereStencil(const SphereGrid& grid);

} // namespace derivean::sim
