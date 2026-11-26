#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Kimberlite{
    .name = "Kimberlite",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x5a6e8c,
    .roughness = 0.90f,
    .density = 2650.0f,
    .compressiveStrength = 70.0f,
    .cohesion = 10000.0f,
    .frictionAngle = 35.0f,
    .slidingFriction = 0.74f,
    .loadCapacity = 250.0f,
    .stableSlope = 50.0f,
    .erodibility = 4e-6f,
    .abrasion = 0.04f,
    .weathering = 60.0f,
    .bedding = 0.05f,
    .jointSpacing = 1.5f,
    .weathersTo = {.blocks = 0.15f, .gravel = 0.20f, .sand = 0.15f, .silt = 0.15f, .clay = 0.35f},
    .porosity = 0.05f,
    .conductivity = 1e-8f,
    .thermalConductivity = 2.2f,
    .heatCapacity = 880.0f,
    .solidus = 1250.0f,
    .liquidus = 1650.0f,
    .massCohesion = 120.0f,
    .massFriction = 30.0f,
};

} // namespace derivean::sim::definition
