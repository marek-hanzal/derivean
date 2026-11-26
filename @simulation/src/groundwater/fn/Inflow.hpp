#pragma once

#include "groundwater/type/Regional.hpp"
#include "groundwater/type/WaterField.hpp"

#include <vector>

namespace derivean::sim {

// The Local end of the regional flow, explicitly. The upwelling of each region cell goes to the
// 16 × 16 local cells under it: to those of them that drain (AquiferColumn::drain), by their
// stream (at least DrainingFlow each, so standing water counts), or else all to the lowest of
// them, into the lower storey. Sets AquiferColumn::regional and WaterField::upwelled of every cell
// of the field, its margin included. A field holds whole region cells, and what a cell gets
// depends on the columns of its region cell alone, so every field that holds a cell gives it the
// same inflow, except within a region cell of the field's edge, where the columns' slope is
// clamped (constant/Hydraulics.hpp counts those rings). `regions` holds the region tiles under
// the field (fn/Regional.hpp); a region cell whose tile is missing sends nothing.
void addRegionalInflow(WaterField& field, const std::vector<const RegionalTile*>& regions);

} // namespace derivean::sim
