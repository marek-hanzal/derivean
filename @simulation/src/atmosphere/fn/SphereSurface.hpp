#pragma once

#include "atmosphere/type/SphereFlow.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// The terrain's squeeze of the column (S4.3d; S4 plan, point 3): the background the flow carries
// with ζ becomes f · (1 + min(h, SphereTerrainLimit) / SphereTerrainDepth) per cell, f = 2Ω sin φ, from
// `height` (m above sea level per cell, finite and ≥ 0; empty for sea level everywhere, which gives
// f, makeSphereFlow's, bit for bit). Over a mountain the column is shorter, so the same absolute
// vorticity spins it less: the shallow layer's potential vorticity (ζ + f) / H with H = D − h,
// to first order in h / D and for ζ ≪ f (the ζ h / D term left out) the f h / D added here (Holton &
// Hakim 2012, ch. 4), the flat world's `absolute`. Throws std::invalid_argument before anything
// changes when the height is not one finite value ≥ 0 per cell or the flow is not of this grid. The
// caller passes the same height as the closures' friction (makeSphereClosures); nothing ties the two
// until the globe's surface owns it (S4.5). It replaces the background whole, so it may be called again with new terrain;
// the background is fixed for the flow within a window (SphereFlowStart does not hold it), so set it
// between windows.
void setSurface(SphereFlow& flow, const SphereGrid& grid, const std::vector<double>& height);

} // namespace derivean::sim
