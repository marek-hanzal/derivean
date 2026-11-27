#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// The plates' regions on the mantle grid: a cell belongs to the plate of the piece that covers most of
// it (the coupling's dominant piece, as `owner`; a bare cell to none), and the cells of one plate joined
// across cell edges or corners (the ring) are its regions. A whole-cell piece offset by a diagonal covers
// most of two cells that share a corner only, so regions joined by edges alone would cut a piece in two.
struct PlateRegions {
    std::vector<int32_t> owner;     // per cell: plate, or -1 for a bare cell
    std::vector<int32_t> region;    // per cell: region, or -1
    std::vector<int32_t> plate;     // per region
    std::vector<double> area;       // per region, sr of its cells
    std::vector<int32_t> cells;     // per region
    std::vector<int32_t> largest;   // per plate: its largest region (the first of equal ones), or -1
};
PlateRegions plateRegionsOf(const Globe& globe);

} // namespace derivean::sim
