#pragma once

#include "planet/fn/Band.hpp"
#include "planet/fn/Boundary.hpp"

namespace derivean::sim {

// A plate reorganisation at the start of an epoch (Codex DREV-CLAUDE-005-04; an addition to
// S3-design.md §4 by the lead's decision of 27 Nov, docs/handoff/planet-globe.md). The band of the
// ending epoch's boundaries is read from the coupling, every piece is dealt to its new plate
// (enterGlobeEpoch), and the boundaries that now lie inside one plate close as Earth's abandoned ones
// do, as sutures and fossil ridges:
// - a piece that pieces of its own plate cover by half or more is consumed by a transfer plan through
//   the accounts of a boundary (ocean into the interior, crust stacked onto the pieces over it within
//   StackLimit, the rest into its plate around it); nothing brakes, since nothing moves apart;
// - then every slot inside one plate next to the old band and open by half or more gets a new piece of
//   that plate (fillSlot): the last spreading of a fossil ridge.
// What stays are gaps and overlaps under half a slot between two lattices of one plate, which no rigid
// plate changes in its own frame (§4, cover slip): whole-cell pieces of two lattices offset by a share f
// of a cell meet with a gap f or an overlap 1 − f. Only pieces of two lattices of one plate can overlap
// (sameLattice), so only those are looked at.
void reorganisePlates(Globe& globe, int step);

// The closing alone, over a band marked before the pieces were dealt to their new plates; whether it
// settled: a round changed nothing, within SeamRounds, and no seam was refused for want of room
// (Codex DREV-CLAUDE-005-05). A refused piece stays half covered by its own plate (stats.seamsKept).
bool closeSeams(Globe& globe, Band& band, int step);

// What the sutures of a closing did: pieces consumed, and pieces left half covered by their own plate
// because the plan found no room for their crust within StackLimit when the last suture ended.
struct Sutures {
    size_t consumed{}, refused{};
};

// The sutures alone in a substep of the boundary band, after its births: a seam that the band's births
// or welds have covered by half again closes as at the reorganisation (lead, 27 Nov); the pieces it
// consumed. Unlike consumption at a boundary nothing holds a suture back, so it may open a mantle cell
// by more than half inside a plate; a slot it opens by half is filled in the substep's next round.
Sutures sutureBand(Globe& globe, Band& band, const Substep& substep);

} // namespace derivean::sim
