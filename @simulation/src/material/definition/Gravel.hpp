#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Gravel{
    .name = "Gravel",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0xb3a78c,
    .roughness = 0.90f,
    .density = 1850.0f,
    .frictionAngle = 36.0f,
    .slidingFriction = 0.73f,
    .loadCapacity = 26.0f,
    .stableSlope = 36.0f,
    .erodibility = 4e-5f,
    .abrasion = 0.015f,
    .weathersTo = {.sand = 0.80f, .silt = 0.12f, .clay = 0.08f},
    .grainSize = 0.02f,
    .porosity = 0.30f,
    .conductivity = 1e-2f,
    .thermalConductivity = 0.7f,
    .heatCapacity = 840.0f,
    .solidus = 1150.0f,
    .liquidus = 1500.0f,
    .massCohesion = 0.0f,
    .massFriction = 36.0f,
};

} // namespace derivean::sim::definition
