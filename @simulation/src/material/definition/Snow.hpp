#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Snow{
    .name = "Snow",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0xf4f7fa,
    .roughness = 0.95f,
    .density = 350.0f,
    .cohesion = 1.0f,
    .frictionAngle = 38.0f,
    .slidingFriction = 0.30f,
    .loadCapacity = 8.0f,
    .stableSlope = 38.0f,
    .erodibility = 1e-3f,
    .grainSize = 1e-3f,
    .porosity = 0.6f,
    .conductivity = 1e-4f,
    .thermalConductivity = 0.25f,
    .heatCapacity = 2090.0f,
    .solidus = 273.15f,
    .liquidus = 273.15f,
    .massCohesion = 1.0f,
    .massFriction = 38.0f,
};

} // namespace derivean::sim::definition
