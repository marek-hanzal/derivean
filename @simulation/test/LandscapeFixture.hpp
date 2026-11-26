#pragma once

#include "measure/fn/History.hpp"
#include "simulation/type/World.hpp"

// One young world shared by the checks that need finished tiles; building it once keeps the run short.
derivean::sim::World& testWorld();

// A point on high ground whose Site tile, Local tile and Region tile are all solved in the first
// pass and lie inside their parents, so reaching it costs exactly one tile per level.
derivean::sim::Vec2 testFocus();

// The middle of the first-pass ground tile just south-east of the focus: voxels and the inspector
// can be asked there at the cost of that one ground tile.
derivean::sim::Vec2 testSpot();

// The planet long after the present. It needs as long as every other landscape check together and
// none of their results, so it is evolved on a thread of its own from the first use of testWorld.
const derivean::sim::PlanetSummary& farFuture();

void expectLandscape(bool passed, const char* message, double value = 0);
