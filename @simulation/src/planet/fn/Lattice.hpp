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
