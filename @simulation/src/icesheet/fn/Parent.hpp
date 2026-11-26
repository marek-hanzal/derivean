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

// The parent's last-sheet deposits (till, moraine, outwash, esker) set aside from a finer domain's
// seeded cover, by class as far as the cover outside the wind's labels holds them, on the ground it
// settles and on both ends of what its followed cells move between; its surface and lake floors
// are held towards the parent's without them. All of it is laid back when its last sheet leaves.
void holdBackLastSheet(Domain& domain, const std::function<const Tile*(TileKey)>& tiles);
void restoreLastSheet(Domain& domain);
// What of the laid-back deposits the domain's own sheet left in place (`left.laidBack`). The tunnels
// took gravel and sand by class between `before` and `eskers`; the ice then stripped the cover from
// the top, where they lie, down to the ground as it is. Null: the sheet took nothing.
void keepLaidBack(const Domain& domain, SheetWork& work, const Ground* before, const Ground* eskers);

// The sheet record of the finished neighbour that owns a halo cell, and the cell's index in it;
// null where no finished neighbour with a record owns the cell.
const SheetRecord* finishedSheet(const Domain& domain, int x, int z, size_t& at);

} // namespace derivean::sim
