#pragma once

#include "icesheet/type/Parent.hpp"
#include "icesheet/type/Sheet.hpp"

#include <functional>
#include <vector>

namespace derivean::sim {

// Parent.cpp — what the level above holds of the sheets, read on the cells of a finer domain.

ParentSheet readParentSheet(const Domain& domain, const std::function<const Tile*(TileKey)>& tiles);

// Stretches of tunnel fill whose parent cell lies under the domain.
std::vector<Tunnel> parentTunnels(const Domain& domain, const std::function<const Tile*(TileKey)>& tiles);

// The sheet record of the finished neighbour that owns a halo cell, and the cell's index in it;
// null where no finished neighbour with a record owns the cell.
const SheetRecord* finishedSheet(const Domain& domain, int x, int z, size_t& at);

} // namespace derivean::sim
