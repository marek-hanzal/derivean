#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Bedrock{
    .name = "Bedrock",
    .phase = Phase::Solid,
    .origin = Origin::Metamorphic,
    .color = 0x414953,
    .roughness = 0.90f,
    .density = 3100.0f,
    .compressiveStrength = 300.0f,
    .cohesion = 60000.0f,
    .frictionAngle = 55.0f,
    .slidingFriction = 0.92f,
    .loadCapacity = 10000.0f,
    .stableSlope = 70.0f,
    .erodibility = 8e-7f,
    .abrasion = 0.005f,
    .weathering = 10.0f,
    .bedding = 0.10f,
    .jointSpacing = 3.0f,
    .weathersTo = {.blocks = 0.35f, .gravel = 0.20f, .sand = 0.25f, .silt = 0.08f, .clay = 0.12f},
    .porosity = 0.005f,
    .conductivity = 1e-10f,
    .thermalConductivity = 3.0f,
    .heatCapacity = 800.0f,
    .solidus = 1300.0f,
    .liquidus = 1600.0f,
    .massCohesion = 400.0f,
    .massFriction = 35.0f,
};

} // namespace derivean::sim::definition
