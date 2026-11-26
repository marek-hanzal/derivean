#pragma once

#include "geometry/type/Cell.hpp"

namespace derivean::sim {

// The edge a flux crosses. The gnomonic grid is not orthogonal (Putman & Lin 2007): the direction from
// one cell centre to the next is not the edge's normal in general (23° apart on face 0, level 5, cell
// (29, 29), edge 1). A finite-volume operator therefore evaluates each shared edge once, at its midpoint
// along its normal, and applies the flux to both cells with opposite signs. Non-orthogonal metric terms
// of a gradient stay with the solver.
CellEdge edgeOf(const Cell& cell, int edge);

} // namespace derivean::sim
