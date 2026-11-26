#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Clay{
    .name = "Clay",
    .phase = Phase::Solid,
    .origin = Origin::Unconsolidated,
    .color = 0xb26d53,
    .roughness = 0.93f,
    .density = 1900.0f,
    .compressiveStrength = 0.3f,
    .cohesion = 50.0f,
    .frictionAngle = 20.0f,
    .slidingFriction = 0.72f,
    .loadCapacity = 130.0f,
    .stableSlope = 45.0f,
    .erodibility = 5e-5f,
    .grainSize = 2e-6f,
    .porosity = 0.50f,
    .conductivity = 1e-9f,
    .thermalConductivity = 1.2f,
    .heatCapacity = 920.0f,
    .solidus = 1400.0f,
    .liquidus = 1600.0f,
    .massCohesion = 15.0f,
    .massFriction = 18.0f,
};

} // namespace derivean::sim::definition
