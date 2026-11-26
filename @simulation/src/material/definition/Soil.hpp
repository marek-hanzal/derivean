#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Soil{
    .name = "Soil",
    .phase = Phase::Solid,
    .origin = Origin::Unconsolidated,
    .color = 0x947459,
    .roughness = 1.0f,
    .density = 1550.0f,
    .compressiveStrength = 0.15f,
    .cohesion = 12.0f,
    .frictionAngle = 30.0f,
    .slidingFriction = 0.85f,
    .loadCapacity = 95.0f,
    .stableSlope = 40.0f,
    .erodibility = 6e-5f,
    .grainSize = 5e-5f,
    .porosity = 0.45f,
    .conductivity = 1e-5f,
    .thermalConductivity = 0.9f,
    .heatCapacity = 1100.0f,
    .solidus = 1350.0f,
    .liquidus = 1650.0f,
    .massCohesion = 8.0f,
    .massFriction = 28.0f,
};

} // namespace derivean::sim::definition
