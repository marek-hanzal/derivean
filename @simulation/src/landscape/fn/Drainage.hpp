#pragma once

#include "landscape/type/Setting.hpp"

namespace derivean::sim {

// The planet's rivers routed over the ground the region window starts from (Backdrop::pastElevation
// at Drainage::Cell), carrying what the planet's own rivers (Backdrop::discharge) gather.
// Rows of the ground are sampled over `workers` when given; the result does not depend on it.
Drainage drainPlanet(const Planet& planet, const Backdrop& backdrop, Workers* workers = nullptr);

} // namespace derivean::sim
