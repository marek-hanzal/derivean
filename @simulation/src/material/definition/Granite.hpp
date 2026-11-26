#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Granite{
    .name = "Granite",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0xbc9c95,
    .roughness = 0.85f,
    .density = 2650.0f,
    .compressiveStrength = 170.0f,
    .cohesion = 35000.0f,
    .frictionAngle = 55.0f,
    .slidingFriction = 0.84f,
    .loadCapacity = 310.0f,
    .stableSlope = 68.0f,
    .erodibility = 1.2e-6f,
    .abrasion = 0.006f,
    .weathering = 25.0f,
    .bedding = 0.05f,
    .jointSpacing = 3.0f,
    .weathersTo = {.blocks = 0.30f, .gravel = 0.15f, .sand = 0.40f, .silt = 0.05f, .clay = 0.10f},
    .porosity = 0.01f,
    .conductivity = 1e-8f,
    .thermalConductivity = 2.9f,
    .heatCapacity = 790.0f,
    .solidus = 1000.0f,
    .liquidus = 1300.0f,
    .massCohesion = 350.0f,
    .massFriction = 34.0f,
};

} // namespace derivean::sim::definition
