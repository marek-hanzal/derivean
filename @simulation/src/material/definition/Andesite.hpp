#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Andesite{
    .name = "Andesite",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x8c7375,
    .roughness = 0.86f,
    .density = 2600.0f,
    .compressiveStrength = 150.0f,
    .cohesion = 30000.0f,
    .frictionAngle = 45.0f,
    .slidingFriction = 0.84f,
    .loadCapacity = 275.0f,
    .stableSlope = 65.0f,
    .erodibility = 1.6e-6f,
    .abrasion = 0.009f,
    .weathering = 22.0f,
    .bedding = 0.30f,
    .jointSpacing = 1.0f,
    .weathersTo = {.blocks = 0.30f, .gravel = 0.25f, .sand = 0.15f, .silt = 0.10f, .clay = 0.20f},
    .porosity = 0.06f,
    .conductivity = 5e-8f,
    .thermalConductivity = 1.8f,
    .heatCapacity = 820.0f,
    .solidus = 1170.0f,
    .liquidus = 1400.0f,
    .massCohesion = 220.0f,
    .massFriction = 33.0f,
};

} // namespace derivean::sim::definition
