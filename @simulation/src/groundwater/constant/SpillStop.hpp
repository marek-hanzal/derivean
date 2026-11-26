#pragma once

#include "simulation/type/InfluenceGraph.hpp"

namespace derivean::sim {

// A spill can revisit a cell after crossing into another catchment. A nonempty
// queue at this fault limit is a model error.
inline constexpr QueueStop HollowSpillStop{64, 64};

} // namespace derivean::sim
