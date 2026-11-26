#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

// Glass that lava shattered into as water quenched it, cemented as it altered to palagonite.
inline constexpr MaterialDefinition Hyaloclastite{
    .name = "Hyaloclastite",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x8f7c45,
    .roughness = 0.95f,
    .density = 2000.0f,
    .compressiveStrength = 15.0f,
    .cohesion = 2500.0f,
    .frictionAngle = 35.0f,
    .slidingFriction = 0.72f,
    .loadCapacity = 150.0f,
    .stableSlope = 55.0f,
    .erodibility = 7e-6f,
    .abrasion = 0.12f,
    .weathering = 120.0f,
    .bedding = 0.35f,
    .jointSpacing = 3.0f,
    .weathersTo = {.blocks = 0.05f, .gravel = 0.20f, .sand = 0.35f, .silt = 0.20f, .clay = 0.20f},
    .porosity = 0.25f,
    .conductivity = 1e-5f,
    .thermalConductivity = 1.2f,
    .heatCapacity = 880.0f,
    .solidus = 1270.0f,
    .liquidus = 1500.0f,
    .massCohesion = 60.0f,
    .massFriction = 32.0f,
};

} // namespace derivean::sim::definition
