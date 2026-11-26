#pragma once

#include "landscape/fn/Process.hpp"

namespace derivean::sim {

// Ground that freezes and thaws every year. Both processes only move the mantle between cells of
// the tile; neither adds nor removes any.
//  - Sorting: freezing ground pushes stones towards where stones already are and fines towards
//    fines, so a mixed mantle separates into stone borders around fine centres; on a slope the
//    borders are drawn out down the fall line into stripes.
//  - Solifluction: the thawed layer flows on the frozen ground below it, faster where it is thick,
//    so thick mantle overrides thin and stands as a lobe with a steep front.
// `sorted` is set to 1 on the cells the frost sorted: their fine centres carry no soil. True if
// either process moved anything.
bool workFrozenGround(Domain& domain, FlowScratch& scratch, std::vector<uint8_t>& sorted);

// Share of the year the ground of one cell spends in the frost-cracking window, from its own
// annual mean: what decides cell by cell where frost has left a block field.
float frostAt(const Domain& domain, size_t cell);

} // namespace derivean::sim
