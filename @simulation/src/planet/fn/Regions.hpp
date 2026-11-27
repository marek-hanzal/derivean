#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

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
// covers of its cells, each cell at most whole. For a plate in two regions or more it is the union of
// the plate's real footprints clipped to each cell (Codex DREV-CLAUDE-008-02 and -05, lead 27 Nov), so
// its largest region and its microplates are chosen by the crust that stands in the region's cells: the
// overlaps summed per cell count crust twice where pieces of a plate overlap, and with cloud-in-cell
// weights they bound nothing. Which cells a region has is still read from the coupling's dominant
// piece, cloud-in-cell outside the band. For a plate in one region, which no area decides, it is that sum.
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
