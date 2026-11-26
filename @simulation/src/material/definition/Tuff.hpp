#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Tuff{
    .name = "Tuff",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0xe3d0a8,
    .roughness = 0.97f,
    .density = 1750.0f,
    .compressiveStrength = 25.0f,
    .cohesion = 4000.0f,
    .frictionAngle = 32.0f,
    .slidingFriction = 0.74f,
    .loadCapacity = 170.0f,
    .stableSlope = 45.0f,
    .erodibility = 9e-6f,
    .abrasion = 0.10f,
    .weathering = 80.0f,
    .bedding = 0.50f,
    .jointSpacing = 2.0f,
    .weathersTo = {.blocks = 0.05f, .gravel = 0.15f, .sand = 0.30f, .silt = 0.25f, .clay = 0.25f},
    .porosity = 0.30f,
    .conductivity = 1e-6f,
    .thermalConductivity = 0.9f,
    .heatCapacity = 900.0f,
    .solidus = 1050.0f,
    .liquidus = 1300.0f,
    .massCohesion = 80.0f,
    .massFriction = 28.0f,
};

} // namespace derivean::sim::definition
