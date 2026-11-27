#pragma once

#include "planet/fn/Band.hpp"

namespace derivean::sim {

// Moves the plates through one planet step of `dt` (yr), step number `step` (for the keys of new
// pieces): the applied Euler vectors, the band, then in each substep the rotation, the exact coupling
// of the band, consumption by a stable transfer plan and births where a contact opens (S3-design.md
// §3–§4). Ends with every piece turned, the consumed gone, the pieces in key order and the coupling
// of the whole globe (exact for the band's pieces, cloud-in-cell for the rest). `refine` multiplies the
// substeps the motion asks for, so a convergence test can halve them at the same step (Codex V4).
void advancePlates(Globe& globe, double dt, int step, int refine = 1);

// What the consumption and the births of a substep need to know about it.
struct Substep {
    int step{}, index{};
    double duration{};   // yr
};

// Whether `donor` is the denser of two crusts and sinks under `survivor` (S3-design.md §3): ocean under
// continent, older ocean under younger (the higher plate index on a tie); two continents do not sink.
bool sinksUnder(const Globe& globe, int donor, int survivor);

// The unit tangent at `from` along the great circle towards `to`.
Vec3 towards(Vec3 from, Vec3 to);

// planet/fn/Consume.cpp: the pieces overriding plates cover by half or more, the plan, its booking.
void consumeInBand(Globe& globe, Band& band, const Substep& substep);

// planet/fn/Spread.cpp: new pieces in deciding cells uncovered by half or more where a contact opens.
void bearInBand(Globe& globe, Band& band, const Substep& substep);

// Fills a cell left open by half or more where no contact opens (Codex DREV-CLAUDE-005-04): a seam
// between two lattices of one plate, or a hole at a closing or sliding contact that consumption did not
// make (holdOpening keeps it from making one). A new piece of `plate` goes into the slot of its lattice
// that covers most of the cell, else onto the cell itself, with crust necked from continental
// neighbours or ocean floor (layNewPiece).
bool fillCell(Globe& globe, Band& band, int cell, int plate, PieceKey key, double bornAt);

// Lays `fresh` (key, plate, birth cell, rotation, area and bornAt set) over `cell` and joins it to the
// band: crust necked from its plate's continental neighbours across the cell's edges when `stretch`
// (S3-design.md §4), else ocean floor, 7 km of basalt from the interior.
void layNewPiece(Globe& globe, Band& band, CrustPiece fresh, int cell, bool stretch);

// The end of a step's or a reorganisation's band work: the consumed gone, the pieces in key order and
// the coupling of the whole globe, exact for the band's pieces and cloud-in-cell for the rest.
void settleBand(Globe& globe, const Band& band);

inline constexpr float StackLimit = 64000.0f;   // m of crust a collision can stack before it backs up

} // namespace derivean::sim
