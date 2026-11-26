#pragma once

#include "groundwater/type/SpillStop.hpp"
#include "landscape/type/Terrain.hpp"

#include <vector>

namespace derivean::sim {

// After routeWater (landscape/fn/Flow.cpp): water an open hollow of soluble rock could not take
// fills the hollow's catchment up to its lowest pass and runs on from there in the same step,
// along the step's shares (`share`, cells × 8), adding to each cell's discharge and, where given,
// to `gathered`. A lake that runs over into one filled earlier in the step joins it, so two
// hollows never hand the same water back and forth; water reaching a finished neighbour's cell has
// left the domain. Water reaching a hollow takes what its floor still can; the rest runs on. Off a
// hollow it sinks no further this step: streams below take in only what reached them in the route.
// Water only ever runs downhill or over a pass out of a catchment, so the spill settles; a queue that
// does not is a fault and throws, rather than drop water.
void spillHollows(Domain& domain, const std::vector<float>& share, std::vector<float>* gathered,
                  const QueueStop& stop = HollowSpillStop);

} // namespace derivean::sim
