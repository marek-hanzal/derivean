#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Air{
    .name = "Air",
    .phase = Phase::Gas,
    .origin = Origin::Void,
    .color = 0x000000,
    .roughness = 1.0f,
    .thermalConductivity = 0.026f,
    .heatCapacity = 1005.0f,
};

} // namespace derivean::sim::definition
