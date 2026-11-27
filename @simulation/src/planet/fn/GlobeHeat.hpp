#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// Melt still cooling under every cell of the mantle grid (the flat Heat.cpp refreshMeltHeat): each
// cell's view of exp(−age / 3 Myr) · min(1, melt / 600 m) over its pieces, spread over the belt by the
// flat [¼ ½ ¼]² kernel read on the cells round a cell (centre ¼, edge neighbours ⅛, corners 1/16),
// weighted by area and normalised, so a cube vertex with three corners keeps a constant field.
void refreshGlobeMeltHeat(Globe& globe);

} // namespace derivean::sim
