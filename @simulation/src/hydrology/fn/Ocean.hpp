#pragma once

#include "hydrology/type/Ocean.hpp"

namespace derivean::sim {

struct Planet;

Ocean fillOcean(const Planet& planet, double age);

// Adds an opening; what has gathered is integrated first, since it ran without it.
void openVoid(Ocean& ocean, const VoidOpening& opening);

// Lets `seconds` pass for the openings: orifice flow under the water column above each mouth,
// integrated exactly between the heights at which the wet area or the connection changes. Time is
// gathered and integrated a day at a time: a tick drains less than the stock can resolve, so
// ticks integrated one by one would drift. `settleOcean` integrates what has gathered at once;
// call it before reading the sea for a budget.
void drainOcean(Ocean& ocean, double seconds);
void settleOcean(Ocean& ocean);

Tally stockOf(const WaterGrid& grid);

// Bodies by falling volume. With no opening, `seaBodies(ocean)[0]` is the world ocean.
std::vector<SeaBody> seaBodies(const Ocean& ocean);

// What the air and the landscape may read. Planet cell indices wrap.
double seaDepthAt(const Ocean& ocean, int x, int z);     // m of water over the planet cell; 0 where it is dry
double seaLevelAt(const Ocean& ocean, int x, int z);     // m, surface of that water; the bed where it is dry
bool openWaterAt(const Ocean& ocean, int x, int z);

// m, surface of the deepest water within one planet cell of a world position already mapped onto
// the planet grid; WorldFloor when no sea is left there. Coasts read this for their present shore.
double seaLevelNear(const Ocean& ocean, Vec2 planetPosition);

} // namespace derivean::sim
