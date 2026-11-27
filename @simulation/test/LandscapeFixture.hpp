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

// The present world (seed 42, 4.6 Gyr) for the checks that need today's glaciated ranges and coasts.
derivean::sim::World& testWorldPresent();

// The planet long after the present, evolved on one thread of its own. The runner starts it ahead
// when testPlanetFuture is among the chosen checks, so that it overlaps them; farFuture waits for it.
void startFarFuture();
const derivean::sim::PlanetSummary& farFuture();

void expectLandscape(bool passed, const char* message, double value = 0);
