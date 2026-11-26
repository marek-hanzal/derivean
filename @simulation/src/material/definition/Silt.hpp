#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Silt{
    .name = "Silt",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0xc4bd84,
    .roughness = 0.99f,
    .density = 1450.0f,
    .cohesion = 2.0f,
    .frictionAngle = 30.0f,
    .slidingFriction = 0.58f,
    .loadCapacity = 18.0f,
    .stableSlope = 30.0f,
    .erodibility = 1.5e-4f,
    .grainSize = 2e-5f,
    .porosity = 0.45f,
    .conductivity = 1e-6f,
    .thermalConductivity = 0.5f,
    .heatCapacity = 850.0f,
    .solidus = 1400.0f,
    .liquidus = 1700.0f,
    .massCohesion = 2.0f,
    .massFriction = 30.0f,
};

} // namespace derivean::sim::definition
