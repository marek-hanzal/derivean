#pragma once

#include "icesheet/fn/Wear.hpp"

namespace derivean::sim {

// Meltwater.cpp — water under and beyond the ice of one stage.

// Water under ice does not run down the bed: it is pushed by the weight of the ice above it, so it
// follows the fall of bed + 0.917 · thickness, eleven parts ice surface to one part bed, and climbs
// a hill the ice surface falls across. Fills `flow` (m³/yr through each cell under grounded ice)
// and `to` (the neighbour it leaves by, 255 where it stops or the cell is bare).
void routeMeltwater(const Domain& domain, const SheetField& field, std::vector<float>& flow, std::vector<uint8_t>& to);

// Where a stream comes out from under the ice it takes part of what the ice dropped there on with
// it, the more the larger it is, and spreads it over the ground beyond: gravel first, sand
// farther, mud out of the domain. `left.moraine` is reduced by what is taken and `left.outwash`
// receives what settles.
void spreadOutwash(Domain& domain, const SheetField& field, const std::vector<float>& flow, const std::vector<uint8_t>& to, SheetRecord& left, WearTotals& totals);

struct OutwashTotals {
    double settled{};      // m³ laid as outwash
    double carriedOff{};   // m³ the water took past the domain's edge or into a cell it does not evolve
};

// Gravel and sand (`load`, m³ per cell by Coarse and Sand) run down `ground` (1e30 under ice) and
// settle along the way, gravel first. The sea keeps what reaches it; a lake (a hollow the land
// closes; water drains under the ice) spreads it over its floor up to the level it spills at and
// loses the rest over its rim. What settles is added to `outwash` (m) and booked.
OutwashTotals washBeyond(Domain& domain, const std::vector<float>& ground, std::vector<float> load[2], std::vector<float>& outwash);

} // namespace derivean::sim
