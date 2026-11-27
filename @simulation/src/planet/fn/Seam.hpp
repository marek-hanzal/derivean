#pragma once

#include "planet/fn/Band.hpp"

namespace derivean::sim {

// A plate reorganisation at the start of an epoch (Codex DREV-CLAUDE-005-04; an addition to
// S3-design.md §4, docs/handoff/planet-globe.md). The band of the ending epoch's boundaries is read
// from the coupling, every piece is dealt to its new plate (enterGlobeEpoch), and the boundaries that
// now lie inside one plate close as Earth's abandoned ones do, as sutures and fossil ridges:
// - a piece that pieces of its own plate cover by half or more is consumed by a transfer plan through
//   the accounts of a boundary (ocean into the interior, crust stacked onto the pieces over it within
//   StackLimit, the rest into its plate around it); nothing brakes, since nothing moves;
// - then every cell of the old band left open by half or more gets a new piece of the plate that
//   covers most of the cell and its ring, in a slot of that plate's lattice where one fits: the last
//   spreading of a fossil ridge, ocean floor or crust necked from continental neighbours.
// What stays are holes and overlaps under half a cell between two lattices of one plate, which no rigid
// plate changes (§4, cover slip): whole-cell pieces of two lattices offset by a share f of a cell meet
// with a gap f or an overlap 1 − f.
void reorganisePlates(Globe& globe, int step);

// The closing alone, over a band marked before the pieces were dealt to their new plates.
void closeSeams(Globe& globe, Band& band, int step);

} // namespace derivean::sim
