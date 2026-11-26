#pragma once

#include "ground/type/Clast.hpp"
#include "landscape/fn/Build.hpp"

namespace derivean::sim {

struct GroundDetail;

// The parts of a ground tile's replay that belong to its scale alone. The landscape's replay
// calls them in this order; everything else it does is shared with the coarser levels.

// Settle.cpp — allocates what only a ground domain carries and reads the frost its climate gives.
void beginGround(Domain& domain, const Setting& setting);

// Settle.cpp — keeps each cell's traits in step with the parting and the joints it has cut down to.
void refreshPartings(Domain& domain, uint64_t seed);

// Settle.cpp — a river works its bed across its whole width, not only along the line its water is routed by.
void widenChannels(Domain& domain);

// Finish.cpp — on the settled surface: stones, the beds of streams, frozen and scalded ground and
// the water that stands in what they leave. Reads the finished tile off the domain, with its `detail`.
void finishGround(Domain& domain, FlowScratch& scratch, const Setting& setting, const TileLookup& lookup, Tile& tile);

// Water.cpp — the last routing, with stones and spring deposits in the water's way, then the tile
// read off the domain and the level of its water: in every hollow that is fed through the year, and
// from bank to bank of every stream.
void standWater(Domain& domain, FlowScratch& scratch, const std::vector<Clast>& stones, const GroundDetail& detail, Tile& tile);

} // namespace derivean::sim
