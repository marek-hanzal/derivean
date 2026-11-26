#pragma once

#include "landscape/type/Setting.hpp"
#include "landscape/type/Trunks.hpp"

namespace derivean::sim {

// The world's trunk rivers (Trunks): the ground the region window starts from
// (Backdrop::pastElevation) with the roughness and rock quality a region seeds, the planet's own
// yield (Backdrop::discharge) routed over it, and the window's erosion by stream power with the
// levels' exponent against the rock uplift a region gets (forcingAt) and the plate's isostatic
// answer to the erosion, so the rivers gather into the valleys they cut while the ranges rise. The
// routing is the authority; the eroded ground is not what a region reaches. The work is spread over
// `workers` when given; the result does not depend on it.
Trunks routeTrunks(const Setting& setting, Workers* workers = nullptr);

} // namespace derivean::sim
