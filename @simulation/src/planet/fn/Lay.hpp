#pragma once

#include "planet/fn/Band.hpp"
#include "planet/type/Globe.hpp"

// Laying new pieces into the boundary band (planet/fn/Lay.cpp): the crust they take and the slot or cell
// they sit on. Births at an opening contact are bearInBand's (planet/fn/Spread.cpp).
namespace derivean::sim {

// Fills a cell left open by half or more at a closing or sliding contact that consumption did not make
// (Codex DREV-CLAUDE-005-04; holdOpening keeps it from making one). A new piece of the plate of `like`
// goes into the most open slot of its lattice over the cell, else onto the cell itself, with crust
// necked from continental neighbours, or ocean floor as old as `like`'s when `like` is too thin to neck
// (the hole is the trench's rounding, not new floor).
bool fillCell(Globe& globe, Band& band, int cell, int like, PieceKey key, double bornAt);

// Fills the most open slot inside one plate next to a deciding cell, open by half or more
// (planet/fn/Interior.hpp): a seam between two lattices of the plate, or a gap no other plate is near.
// The crust is necked from the slot's continental neighbours, or ocean floor as old as the piece the
// slot continues when that is ocean.
bool fillSlot(Globe& globe, Band& band, int cell, PieceKey key, double bornAt);

// Lays `fresh` (key, plate, birth cell, rotation, area and bornAt set) over `cell` and joins it to the
// band: crust necked from its plate's continental neighbours across the cell's edges when `stretch`
// (S3-design.md §4), else ocean floor, 7 km of basalt from the interior.
void layNewPiece(Globe& globe, Band& band, CrustPiece fresh, int cell, bool stretch);

} // namespace derivean::sim
