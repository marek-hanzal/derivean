#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// The surface processes of the flat Surface.cpp on the globe (S3-design.md §2, §7): the same physics
// with real cell areas, the cells round a cell and geodesic distances. Mass moves as volumes between
// cells and is booked into the pieces over a cell by weight.

// The view's solid surface: per cell Σ w · columnElevation / Σ w over its pieces, plus the swell.
void refreshGlobeElevation(Globe& globe);

// The level L whose water Σ_c A_c · max(0, L − h_c) is the surface water, exactly: the cells sorted by
// height and the piecewise linear volume walked, no bins (still a bathtub, as the flat history).
void solveGlobeSea(Globe& globe);

// Erosion, transport of the loads a few cells per step, deposition by environment and carbonate.
// What cannot settle on a covered cell (a footprint hole) goes to the interior.
void shapeGlobeSurface(Globe& globe, double dt);

// The past mark of every piece at the current anchor, from its own column and the swell under it.
void stampGlobeMarks(Globe& globe);

} // namespace derivean::sim
