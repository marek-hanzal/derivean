#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Gneiss{
    .name = "Gneiss",
    .phase = Phase::Solid,
    .origin = Origin::Metamorphic,
    .color = 0x767e8a,
    .roughness = 0.84f,
    .density = 2750.0f,
    .compressiveStrength = 150.0f,
    .cohesion = 30000.0f,
    .frictionAngle = 50.0f,
    .slidingFriction = 0.83f,
    .loadCapacity = 300.0f,
    .stableSlope = 65.0f,
    .erodibility = 1.0e-6f,
    .abrasion = 0.006f,
    .weathering = 18.0f,
    .bedding = 0.35f,
    .jointSpacing = 2.0f,
    .weathersTo = {.blocks = 0.35f, .gravel = 0.20f, .sand = 0.25f, .silt = 0.08f, .clay = 0.12f},
    .porosity = 0.01f,
    .conductivity = 5e-9f,
    .thermalConductivity = 2.7f,
    .heatCapacity = 800.0f,
    .solidus = 1020.0f,
    .liquidus = 1350.0f,
    .massCohesion = 300.0f,
    .massFriction = 33.0f,
};

} // namespace derivean::sim::definition
