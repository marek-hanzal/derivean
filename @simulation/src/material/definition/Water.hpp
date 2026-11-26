#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Water{
    .name = "Water",
    .phase = Phase::Liquid,
    .origin = Origin::Fluid,
    .color = 0x2f939b,
    .roughness = 0.16f,
    .density = 998.0f,
    .thermalConductivity = 0.598f,
    .heatCapacity = 4182.0f,
    .solidus = 273.15f,
    .liquidus = 373.15f,
    .dynamicViscosity = 0.001002f,
    .surfaceTension = 0.0728f,
};

} // namespace derivean::sim::definition
