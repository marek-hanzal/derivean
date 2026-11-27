#pragma once

#include "geometry/type/Rotation.hpp"
#include "geometry/type/Vec3.hpp"
#include "planet/fn/Band.hpp"
#include "planet/type/Globe.hpp"

#include <vector>

// The lattices of the plates' pieces that new crust continues at a gap (S3-design.md §4, Codex
// DREV-CLAUDE-005-01 and -02): the union of pieces over a cell, the live pieces over it, the slots next
// to a plate's pieces, and the weld that places new crust when no slot fits.
namespace derivean::sim {

inline constexpr double BornShare = 0.5;   // of a cell left uncovered by the union of its pieces
inline constexpr double SlotClash = 0.02;  // of a lattice slot its own plate may already cover (rounding of shared edges)
inline constexpr double SlotHint = 0.1;    // a cell whose overlaps sum to less than 1 − this may hold an open slot
// Of a slot, how far the union of its clipped neighbours may fall short of their largest overlap: the
// union rounds every corner to Clipper2's integer grid (δ = 1e-12 of a plane unit) in another tangent
// plane, at worst 4δ/s of a slot of side s (plane units), about 2.5e-10 at level 7 and 5e-10 at level 8;
// level 9 would reach 1e-9, so fillBand (planet/fn/Band.cpp) requires level 8 at most.
inline constexpr double UnionShortfall = 1e-9;
inline constexpr double SlotFill = 0.25;   // of a slot left open for it to fill an open cell that has no open slot

// The area of a cell the given pieces cover together: the union of their footprints clipped to the
// cell, in its tangent plane (S3-design.md §4); `sum` when there is one piece or none.
double unionInCell(const Globe& globe, int cell, const std::vector<int>& pieces, double sum, long& unions);

// The live pieces over a cell, this substep's births included, with their summed and largest overlap;
// with plate ≥ 0 only that plate's pieces whose centres lie behind x against `normal`.
struct Over {
    std::vector<int> pieces;
    double sum{}, most{};
};
Over overOf(const Globe& globe, const Band& band, int cell, int plate, Vec3 x, Vec3 normal);

// Whether the pieces of `over` cover more than `half` (m²) of the cell: over.sum > half and either
// over.most > half or unionInCell(...) > half. The union is taken only where its bounds leave the answer
// open: it is at least the largest overlap, and at least the sum less the overlaps of every pair of
// pieces from two lattices (pieces of one lattice tile and do not overlap; Bonferroni).
bool coversOver(const Globe& globe, int cell, const Over& over, double half, long& unions);

// How far (rad) to move a new footprint from its cell along `normal` to weld it onto its own plate.
double weldTurn(Globe& globe, const Band& band, int cell, int plate, Vec3 x, Vec3 normal);

// A slot of a plate's lattice next to the gap, on the side of its pieces that `normal` points to (any
// side for a zero normal): its birth cell and the rotation of the piece it continues.
struct Slot {
    int birth{-1};
    Rotation rotation;
};
bool openSlot(Globe& globe, const Band& band, int cell, int plate, Vec3 normal, bool cellOpen, Slot& slot);

} // namespace derivean::sim
