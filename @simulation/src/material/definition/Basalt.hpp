#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Basalt{
    .name = "Basalt",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x4b6062,
    .roughness = 0.79f,
    .density = 2900.0f,
    .compressiveStrength = 200.0f,
    .cohesion = 40000.0f,
    .frictionAngle = 50.0f,
    .slidingFriction = 0.85f,
    .loadCapacity = 280.0f,
    .stableSlope = 70.0f,
    .erodibility = 1.8e-6f,
    .abrasion = 0.008f,
    .weathering = 20.0f,
    .bedding = 0.25f,
    .jointSpacing = 0.6f,
    .weathersTo = {.blocks = 0.30f, .gravel = 0.25f, .sand = 0.10f, .silt = 0.10f, .clay = 0.25f},
    .porosity = 0.05f,
    .conductivity = 1e-7f,
    .thermalConductivity = 1.9f,
    .heatCapacity = 840.0f,
    .solidus = 1270.0f,
    .liquidus = 1500.0f,
    .massCohesion = 250.0f,
    .massFriction = 34.0f,
};

} // namespace derivean::sim::definition
