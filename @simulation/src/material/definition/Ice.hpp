#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Ice{
    .name = "Ice",
    .phase = Phase::Solid,
    .origin = Origin::Fluid,
    .color = 0xbfe0ee,
    .roughness = 0.20f,
    .density = 917.0f,
    .compressiveStrength = 10.0f,
    .cohesion = 1000.0f,
    .slidingFriction = 0.05f,
    .loadCapacity = 140.0f,
    .stableSlope = 60.0f,
    .bedding = 0.6f,
    .jointSpacing = 8.0f,
    .thermalConductivity = 2.2f,
    .heatCapacity = 2090.0f,
    .solidus = 273.15f,
    .liquidus = 273.15f,
    .massCohesion = 150.0f,
    .massFriction = 25.0f,
};

} // namespace derivean::sim::definition
