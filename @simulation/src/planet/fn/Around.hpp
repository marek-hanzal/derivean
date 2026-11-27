#pragma once

#include "planet/fn/Band.hpp"

#include <vector>

namespace derivean::sim {

// Where a suture's donor puts the crust its receivers have no room for (planet/fn/Seam): pieces of its
// own plate, nearest first (the lower key on a tie), never k itself nor a piece in `skip`.

// The live pieces over k's cells and their rings, up to 8: the ordinary reach.
std::vector<int> aroundOf(const Globe& globe, const Band& band, int k, const std::vector<int>& cells, const std::vector<int>& skip);

// The live pieces within WideReach of k, from the band's lists and the globe's coupling, up to
// WideAround: the reach of a donor the ordinary reach refused (Codex DREV-CLAUDE-005-05), the doubled
// crust spreading through its plate before what is left founders (TransferKind::Delaminate).
std::vector<int> wideAroundOf(const Globe& globe, const Band& band, int k, const std::vector<int>& cells, const std::vector<int>& skip);

} // namespace derivean::sim
