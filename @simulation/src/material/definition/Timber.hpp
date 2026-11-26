#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Timber{
    .name = "Timber",
    .phase = Phase::Solid,
    .origin = Origin::Organic,
    .color = 0xb77949,
    .roughness = 0.84f,
    .density = 650.0f,
    .compressiveStrength = 40.0f,
    .cohesion = 8000.0f,
    .slidingFriction = 0.78f,
    .loadCapacity = 160.0f,
    .stableSlope = 90.0f,
    .porosity = 0.55f,
    .conductivity = 1e-9f,
    .thermalConductivity = 0.16f,
    .heatCapacity = 1700.0f,
};

} // namespace derivean::sim::definition
