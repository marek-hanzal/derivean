#pragma once

#include "landscape/type/Terrain.hpp"

// Shared by the coast tests (Coast.cpp, CoastLake.cpp).
namespace derivean::sim { struct CoastSpec; }

// Reports a failed check of the coast and ends the run.
void expectCoast(bool passed, const char* message, double value = 0);
// m³ of loose cover on every cell and of broken rock in every surf.
double looseIn(const derivean::sim::Domain& domain);
// Waves alone on a scene for `steps` steps; checks on the way out that nothing was made or lost.
derivean::sim::Domain worked(const derivean::sim::CoastSpec& spec, int steps);
