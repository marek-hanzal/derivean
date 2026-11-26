#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Moss{
    .name = "Moss",
    .phase = Phase::Solid,
    .origin = Origin::Organic,
    .color = 0x78875a,
    .roughness = 0.96f,
    .density = 1300.0f,
    .compressiveStrength = 0.05f,
    .cohesion = 8.0f,
    .frictionAngle = 28.0f,
    .slidingFriction = 0.85f,
    .loadCapacity = 80.0f,
    .stableSlope = 40.0f,
    .erodibility = 6e-5f,
    .porosity = 0.6f,
    .conductivity = 1e-5f,
    .thermalConductivity = 0.4f,
    .heatCapacity = 1900.0f,
    .massCohesion = 5.0f,
    .massFriction = 28.0f,
};

} // namespace derivean::sim::definition
