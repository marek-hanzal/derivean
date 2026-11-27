#pragma once

#include "landscape/type/Level.hpp"

namespace derivean::sim {

// The pool priorities of a world's builds. A build waits only for builds ranked above it
// (Workers.hpp). The one exception is a terrain tile waiting for its neighbours of the same level,
// which belong to an earlier pass (landscape/fn/Build.cpp, dependencies). Atlas::require lets
// a wait at its own level through; that it follows the passes rests on dependencies alone.
// Terrain: Region 900, Local 800, Site 700, Ground 600; finer tiles read coarser ones.
inline constexpr int terrainPriority(Level level) { return 1000 - 100 * int(level); }

// Ground water ranks just below the terrain it reads: regional 899, local 799. Local water reads
// regional water, and a ground tile reads local water.
inline constexpr int aquiferPriority(Level level) { return terrainPriority(level) - 1; }

// The parts of a prepared build (simulation/fn/BuildGraph.cpp) may wait for any tile or ground
// water, so they rank below all of them.
inline constexpr int preparedPriority = terrainPriority(Level::Ground) - 100;

} // namespace derivean::sim
