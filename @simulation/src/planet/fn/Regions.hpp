#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// Each plate is one region of the mantle grid (lead, 27 Nov; planet/fn/Regions): its cells joined
// through its own and free cells, so only another plate's cells cut it. Earth's plates follow Bird (2003,
// docs/credits): none is smaller than about 0.002 sr, and a plate cut in two by its boundaries is two
// plates. A level-5 cell is 0.0017–0.0024 sr, so a plate is also two cells at least (lead, 27 Nov): one
// cell cut off joins. Both count crust that stands there: the area its plate covers and the cells it owns
// by OwnedShare, not a cell's whole area for a rounding sliver (Codex DREV-CLAUDE-008-01).
inline constexpr double MicroplateArea = 0.002;   // sr
inline constexpr int MicroplateCells = 2;

// A cell is a plate's when the piece covering most of it covers this share of it at least. Less is the
// rounding of the exact overlaps where footprints meet along an edge or a corner (about 1e-16 of a cell,
// geometry/fn/Overlap.hpp) or of the band's normalised weights (about 1e-12), not crust standing on the
// cell (Codex DREV-CLAUDE-008-01). It is a million times the band's rounding and far under the slivers
// a plate's motion leaves (a substep moves a plate kilometres; 1e-6 of a level-5 cell is a strip 0.3 m
// wide along its edge). It classifies cells only: the balance and the coupling take every piece whole.
inline constexpr double OwnedShare = 1e-6;

// The plates' regions on the mantle grid: a cell belongs to the plate of the piece that covers most of
// it, by OwnedShare at least (the coupling's dominant piece, as `owner`; any other cell is free), and the
// cells of one plate joined across cell edges or corners (the ring), directly or through free cells, are
// its regions: a gap inside a plate fills with its own new crust and is no boundary with another plate
// (lead, 27 Nov), and a whole-cell piece offset by a diagonal covers most of two cells that share a corner
// only, so regions joined by edges alone would cut a piece in two. A region's area is what its plate
// covers of its cells, each cell at most whole: first the overlaps of its plate's pieces summed per cell,
// which count crust twice where they overlap each other, so an upper bound of the cover (the lead's
// quick bound, 27 Nov; with cloud-in-cell weights an estimate, not a strict bound), with the largest
// overlap per cell below it. Where they leave a decision open, the area is the union of the plate's real
// footprints in each cell (Codex DREV-CLAUDE-008-02): for a region of MicroplateCells or more that is not
// its plate's largest and whose upper bound reaches MicroplateArea, and for a plate's regions whose upper
// bound reaches the lower bound of the one with the best lower bound (which is the largest unless one of
// their unions reaches it). Any other region's area is its upper bound.
struct PlateRegions {
    std::vector<int32_t> dominant;  // per cell: the coupling's dominant piece, or -1
    std::vector<int32_t> owner;     // per cell: plate, or -1 for a free cell
    std::vector<int32_t> region;    // per cell: region, or -1
    std::vector<int32_t> plate;     // per region
    std::vector<double> area;       // per region, sr its plate covers in its cells
    std::vector<int32_t> cells;     // per region
    std::vector<int32_t> largest;   // per plate: its largest region (the first of equal ones), or -1
};
PlateRegions plateRegionsOf(const Globe& globe);

} // namespace derivean::sim
