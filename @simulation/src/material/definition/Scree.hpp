#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Scree{
    .name = "Scree",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0x93908a,
    .roughness = 0.92f,
    .density = 1750.0f,
    .frictionAngle = 38.0f,
    .slidingFriction = 0.78f,
    .loadCapacity = 30.0f,
    .stableSlope = 38.0f,
    .erodibility = 2e-5f,
    .abrasion = 0.02f,
    .weathersTo = {.gravel = 0.55f, .sand = 0.35f, .silt = 0.05f, .clay = 0.05f},
    .grainSize = 0.4f,
    .porosity = 0.35f,
    .conductivity = 5e-2f,
    .thermalConductivity = 0.6f,
    .heatCapacity = 800.0f,
    .solidus = 1100.0f,
    .liquidus = 1450.0f,
    .massCohesion = 0.0f,
    .massFriction = 38.0f,
};

} // namespace derivean::sim::definition
