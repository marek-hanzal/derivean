#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

// Basalt that set under water as stacked pillows, each with a glassy rind and open space between.
inline constexpr MaterialDefinition PillowLava{
    .name = "PillowLava",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x3b4a48,
    .roughness = 0.85f,
    .density = 2750.0f,
    .compressiveStrength = 120.0f,
    .cohesion = 22000.0f,
    .frictionAngle = 45.0f,
    .slidingFriction = 0.80f,
    .loadCapacity = 260.0f,
    .stableSlope = 65.0f,
    .erodibility = 2.5e-6f,
    .abrasion = 0.012f,
    .weathering = 30.0f,
    .bedding = 0.20f,
    .jointSpacing = 0.6f,
    .weathersTo = {.blocks = 0.35f, .gravel = 0.25f, .sand = 0.10f, .silt = 0.10f, .clay = 0.20f},
    .porosity = 0.10f,
    .conductivity = 1e-5f,
    .thermalConductivity = 1.7f,
    .heatCapacity = 840.0f,
    .solidus = 1270.0f,
    .liquidus = 1500.0f,
    .massCohesion = 180.0f,
    .massFriction = 34.0f,
};

} // namespace derivean::sim::definition
