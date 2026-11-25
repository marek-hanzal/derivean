#pragma once

#include "simulation/type/Simulation.hpp"

namespace derivean {

void editTerrain(Simulation& game, Frame& frame, const RayHit& hit, const Command& command);

} // namespace derivean
