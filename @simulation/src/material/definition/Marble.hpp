#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Marble{
    .name = "Marble",
    .phase = Phase::Solid,
    .origin = Origin::Metamorphic,
    .color = 0xe8e4da,
    .roughness = 0.75f,
    .density = 2710.0f,
    .compressiveStrength = 100.0f,
    .cohesion = 20000.0f,
    .frictionAngle = 40.0f,
    .slidingFriction = 0.72f,
    .loadCapacity = 260.0f,
    .stableSlope = 65.0f,
    .erodibility = 1.6e-6f,
    .abrasion = 0.03f,
    .weathering = 10.0f,
    .solubility = 35.0f,
    .bedding = 0.20f,
    .jointSpacing = 2.5f,
    .weathersTo = {.blocks = 0.15f, .gravel = 0.10f, .sand = 0.05f, .silt = 0.05f, .clay = 0.05f, .solute = 0.60f},
    .porosity = 0.01f,
    .conductivity = 1e-7f,
    .thermalConductivity = 2.8f,
    .heatCapacity = 880.0f,
    .solidus = 1170.0f,
    .liquidus = 1610.0f,
    .massCohesion = 280.0f,
    .massFriction = 33.0f,
};

} // namespace derivean::sim::definition
