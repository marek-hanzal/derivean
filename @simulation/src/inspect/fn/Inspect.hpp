#pragma once

#include "inspect/type/Record.hpp"
#include "simulation/type/World.hpp"

#include <utility>

namespace derivean::sim {

// Blocks like surveyAt until the ground tiles under the point exist. The surface and cover are the
// ones surveyAt returns there for the ground level, the column the one sampleBlock returns.
InspectRecord inspectAt(World& world, Vec2 position);

// The planet-scale part alone; needs no tiles.
InspectCrust inspectCrust(const Planet& planet, uint64_t seed, Vec2 position);

// The record as labelled values with units, in reading order, for a generic key and value list.
// Labels are "Group / Name".
std::vector<std::pair<std::string, std::string>> describe(const InspectRecord& record);

const char* nameOf(SurfaceOrigin origin);
const char* nameOf(Regime regime);
const char* nameOf(Basement basement);

} // namespace derivean::sim
