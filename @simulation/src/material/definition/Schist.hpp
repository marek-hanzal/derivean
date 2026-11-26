#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Schist{
    .name = "Schist",
    .phase = Phase::Solid,
    .origin = Origin::Metamorphic,
    .color = 0x6f687b,
    .roughness = 0.70f,
    .density = 2800.0f,
    .compressiveStrength = 80.0f,
    .cohesion = 15000.0f,
    .frictionAngle = 38.0f,
    .slidingFriction = 0.70f,
    .loadCapacity = 230.0f,
    .stableSlope = 50.0f,
    .erodibility = 2.2e-6f,
    .abrasion = 0.04f,
    .weathering = 30.0f,
    .bedding = 0.70f,
    .jointSpacing = 0.8f,
    .weathersTo = {.blocks = 0.20f, .gravel = 0.25f, .sand = 0.20f, .silt = 0.15f, .clay = 0.20f},
    .porosity = 0.02f,
    .conductivity = 1e-8f,
    .thermalConductivity = 3.1f,
    .heatCapacity = 800.0f,
    .solidus = 1000.0f,
    .liquidus = 1350.0f,
    .massCohesion = 120.0f,
    .massFriction = 27.0f,
};

} // namespace derivean::sim::definition
