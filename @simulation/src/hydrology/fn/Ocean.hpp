#pragma once

#include "hydrology/type/Ocean.hpp"

namespace derivean::sim {

struct Planet;

Ocean fillOcean(const Planet& planet, double age);

// The deepest cell of the body holding the most water: the ocean is known by it, not by the
// deepest cell of the planet, which may lie in a small cut-off pit.
int32_t anchorOf(const Ocean& ocean);

// Cuts a strait; what has gathered is integrated first, since it ran without it. Refused (false)
// for ends that coincide or lie off the grid, a throat with no area, and on a grid that drains
// across its rim, where water could leave the bodies.
bool openStrait(Ocean& ocean, const Strait& strait);

// Lets `seconds` pass for the straits: orifice flow from the upper body into the lower, integrated
// exactly between the moments the wet areas, the bodies or the outflow change. Time is gathered and
// integrated a day at a time: a tick moves less than the stocks can resolve, so ticks integrated
// one by one would drift. `settleOcean` integrates what has gathered at once; call it before
// reading the sea for a budget.
void drainOcean(Ocean& ocean, double seconds);
void settleOcean(Ocean& ocean);

Tally stockOf(const WaterGrid& grid);
double crustOf(const WaterGrid& grid);   // kg of salt on the dry cells

// Bodies by falling volume; the world ocean is the one flagged `ocean`.
std::vector<SeaBody> seaBodies(const Ocean& ocean);

// What the air and the landscape may read. Planet cell indices wrap.
double seaDepthAt(const Ocean& ocean, int x, int z);     // m of water over the planet cell; 0 where it is dry
double seaLevelAt(const Ocean& ocean, int x, int z);     // m, surface of that water; the bed where it is dry
bool openWaterAt(const Ocean& ocean, int x, int z);

// m, surface of the deepest water within one planet cell of a world position already mapped onto
// the planet grid; WorldFloor when no sea is left there. Coasts read this for their present shore.
double seaLevelNear(const Ocean& ocean, Vec2 planetPosition);

} // namespace derivean::sim
