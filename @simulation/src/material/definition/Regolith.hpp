#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Regolith{
    .name = "Regolith",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0xc48e6c,
    .roughness = 0.98f,
    .density = 1700.0f,
    .cohesion = 5.0f,
    .frictionAngle = 35.0f,
    .slidingFriction = 0.70f,
    .loadCapacity = 40.0f,
    .stableSlope = 38.0f,
    .erodibility = 5e-5f,
    .weathersTo = {.gravel = 0.10f, .sand = 0.50f, .silt = 0.15f, .clay = 0.25f},
    .grainSize = 2e-3f,
    .porosity = 0.35f,
    .conductivity = 1e-5f,
    .thermalConductivity = 0.6f,
    .heatCapacity = 850.0f,
    .solidus = 1200.0f,
    .liquidus = 1500.0f,
    .massCohesion = 5.0f,
    .massFriction = 35.0f,
};

} // namespace derivean::sim::definition
