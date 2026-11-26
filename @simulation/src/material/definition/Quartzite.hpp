#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Quartzite{
    .name = "Quartzite",
    .phase = Phase::Solid,
    .origin = Origin::Metamorphic,
    .color = 0xcbc5b2,
    .roughness = 0.72f,
    .density = 2650.0f,
    .compressiveStrength = 250.0f,
    .cohesion = 60000.0f,
    .frictionAngle = 58.0f,
    .slidingFriction = 0.80f,
    .loadCapacity = 330.0f,
    .stableSlope = 75.0f,
    .erodibility = 5e-7f,
    .abrasion = 0.002f,
    .weathering = 6.0f,
    .bedding = 0.30f,
    .jointSpacing = 1.5f,
    .weathersTo = {.blocks = 0.55f, .gravel = 0.25f, .sand = 0.18f, .silt = 0.02f},
    .porosity = 0.01f,
    .conductivity = 1e-9f,
    .thermalConductivity = 5.5f,
    .heatCapacity = 750.0f,
    .solidus = 1700.0f,
    .liquidus = 1980.0f,
    .massCohesion = 400.0f,
    .massFriction = 35.0f,
};

} // namespace derivean::sim::definition
