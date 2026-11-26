#pragma once

#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// Surface water of a replay on soluble rock (groundwater/type/Sinking.hpp). Local and Site
// domains; the Region and the ground level take what they are given. The steps run in this order:
//   prepareSinks   after readClimate: what each cell takes in, where each soluble body rises, and
//                  the water that sank in the step before coming out there;
//   drainsDown     in floodLevels: a hollow of soluble rock whose floor took in all it received in
//                  the step before is a way out, not a lake;
//   soakAt         in routeWater, as a cell's own runoff is put on it: the share that soaks into
//                  soluble rock goes down where it falls;
//   sinkAt         in routeWater, before a cell's water is shared out: books what goes down;
//   dissolveSinks  after routeWater: rock dissolves where the water went down, so hollows that
//                  gather water deepen: dolines.
// m above its rising within which soluble ground takes nothing in: the water in the rock stands
// near the ground there, and a valley floor at the level of the spring loses nothing. Twice as
// high it takes in its full rate. Water that has come out at the rising cannot run back into its
// body on the way down.
inline constexpr float SinkRisingZone = 15.0f;

void prepareSinks(Domain& domain);
bool drainsDown(const Domain& domain, size_t cell);
// m³/yr of a cell's own runoff `own` that soaks in and goes down there.
float soakAt(Domain& domain, size_t cell, float own);
// m³/yr of `water` that goes down in `cell`: all of it in an open hollow (`pit`), what the floor of
// the cell takes under standing water, what a stream's wetted strip takes elsewhere.
float sinkAt(Domain& domain, size_t cell, float water, bool standing, bool pit);
void dissolveSinks(Domain& domain);

} // namespace derivean::sim
