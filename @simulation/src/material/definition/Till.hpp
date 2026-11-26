#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Till{
    .name = "Till",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0x877b64,
    .roughness = 0.97f,
    .density = 2100.0f,
    .cohesion = 20.0f,
    .frictionAngle = 40.0f,
    .slidingFriction = 0.84f,
    .loadCapacity = 60.0f,
    .stableSlope = 45.0f,
    .erodibility = 2.5e-5f,
    .abrasion = 0.03f,
    .weathersTo = {.blocks = 0.10f, .gravel = 0.30f, .sand = 0.30f, .silt = 0.15f, .clay = 0.15f},
    .grainSize = 0.01f,
    .porosity = 0.20f,
    .conductivity = 1e-7f,
    .thermalConductivity = 1.6f,
    .heatCapacity = 850.0f,
    .solidus = 1150.0f,
    .liquidus = 1500.0f,
    .massCohesion = 20.0f,
    .massFriction = 40.0f,
};

} // namespace derivean::sim::definition
