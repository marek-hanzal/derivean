#pragma once

#include "simulation/type/Simulation.hpp"

namespace derivean {

void streamColliders(Simulation& game);
void projectScene(Simulation& game, Frame& frame);
void markTerrainChanged(Simulation& game, std::span<const Cell> changed);

} // namespace derivean
