#pragma once

#include "ground/fn/Stones.hpp"
#include "ground/type/Fabric.hpp"

namespace derivean::sim {

// Works out the per-cell fields for the domain the stones are being laid on.
FabricContext fabricContextOf(const StoneBed& stones, const FlowScratch& scratch, const TileLookup& lookup);

// The fabric at a domain cell; `draw` varies what differs from stone to stone. A cover of zero means no stones.
Fabric fabricAt(const FabricContext& context, size_t cell, uint64_t draw);

} // namespace derivean::sim
