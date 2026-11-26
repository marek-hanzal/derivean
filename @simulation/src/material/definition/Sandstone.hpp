#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Sandstone{
    .name = "Sandstone",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0xd09f66,
    .roughness = 0.95f,
    .density = 2250.0f,
    .compressiveStrength = 70.0f,
    .cohesion = 12000.0f,
    .frictionAngle = 35.0f,
    .slidingFriction = 0.80f,
    .loadCapacity = 240.0f,
    .stableSlope = 78.0f,
    .erodibility = 3.0e-6f,
    .abrasion = 0.03f,
    .weathering = 35.0f,
    .solubility = 1.0f,
    .bedding = 0.55f,
    .jointSpacing = 6.0f,
    .weathersTo = {.blocks = 0.12f, .gravel = 0.03f, .sand = 0.80f, .silt = 0.03f, .clay = 0.02f},
    .porosity = 0.18f,
    .conductivity = 1e-5f,
    .thermalConductivity = 2.5f,
    .heatCapacity = 850.0f,
    .solidus = 1350.0f,
    .liquidus = 1850.0f,
    .massCohesion = 250.0f,
    .massFriction = 32.0f,
};

} // namespace derivean::sim::definition
