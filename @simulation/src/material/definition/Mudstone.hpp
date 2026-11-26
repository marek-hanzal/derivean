#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Mudstone{
    .name = "Mudstone",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0x655c53,
    .roughness = 0.94f,
    .density = 2400.0f,
    .compressiveStrength = 25.0f,
    .cohesion = 4000.0f,
    .frictionAngle = 25.0f,
    .slidingFriction = 0.68f,
    .loadCapacity = 190.0f,
    .stableSlope = 30.0f,
    .erodibility = 1.2e-5f,
    .abrasion = 0.25f,
    .weathering = 150.0f,
    .bedding = 0.90f,
    .jointSpacing = 0.3f,
    .weathersTo = {.blocks = 0.02f, .gravel = 0.05f, .sand = 0.08f, .silt = 0.35f, .clay = 0.50f},
    .porosity = 0.10f,
    .conductivity = 1e-10f,
    .thermalConductivity = 1.8f,
    .heatCapacity = 900.0f,
    .solidus = 1050.0f,
    .liquidus = 1450.0f,
    .massCohesion = 35.0f,
    .massFriction = 22.0f,
};

} // namespace derivean::sim::definition
