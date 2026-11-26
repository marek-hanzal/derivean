#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Coal{
    .name = "Coal",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0x222124,
    .roughness = 0.65f,
    .density = 1350.0f,
    .compressiveStrength = 20.0f,
    .cohesion = 2000.0f,
    .frictionAngle = 30.0f,
    .slidingFriction = 0.58f,
    .loadCapacity = 120.0f,
    .stableSlope = 35.0f,
    .erodibility = 1.5e-5f,
    .abrasion = 0.30f,
    .weathering = 120.0f,
    .bedding = 0.90f,
    .jointSpacing = 0.2f,
    .weathersTo = {.blocks = 0.05f, .gravel = 0.15f, .sand = 0.10f, .silt = 0.30f, .clay = 0.40f},
    .porosity = 0.05f,
    .conductivity = 1e-8f,
    .thermalConductivity = 0.3f,
    .heatCapacity = 1300.0f,
    .massCohesion = 25.0f,
    .massFriction = 24.0f,
};

} // namespace derivean::sim::definition
