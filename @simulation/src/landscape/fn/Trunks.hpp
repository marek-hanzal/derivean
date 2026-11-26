#pragma once

#include "landscape/type/Setting.hpp"
#include "landscape/type/Trunks.hpp"

namespace derivean::sim {

// The world's trunk rivers (Trunks): the ground the region window starts from
// (Backdrop::pastElevation) with the roughness a region seeds, the planet's own yield
// (Backdrop::discharge) routed over it, and the window's erosion by stream power with the levels'
// exponent (one rock, no uplift), so the rivers gather into the valleys they cut. The routing is the
// authority; the eroded ground is not what a region reaches. Rows of the ground are sampled over `workers`
// when given; the result does not depend on it.
Trunks routeTrunks(const Planet& planet, const Backdrop& backdrop, Workers* workers = nullptr);

} // namespace derivean::sim
