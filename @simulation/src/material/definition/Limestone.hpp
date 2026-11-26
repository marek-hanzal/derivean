#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Limestone{
    .name = "Limestone",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0xaab0ad,
    .roughness = 0.88f,
    .density = 2500.0f,
    .compressiveStrength = 90.0f,
    .cohesion = 18000.0f,
    .frictionAngle = 40.0f,
    .slidingFriction = 0.76f,
    .loadCapacity = 250.0f,
    .stableSlope = 72.0f,
    .erodibility = 2.4e-6f,
    .abrasion = 0.03f,
    .weathering = 12.0f,
    .solubility = 45.0f,
    .bedding = 0.50f,
    .jointSpacing = 3.0f,
    .weathersTo = {.blocks = 0.12f, .gravel = 0.08f, .sand = 0.03f, .silt = 0.05f, .clay = 0.07f, .solute = 0.65f},
    .porosity = 0.08f,
    .conductivity = 1e-7f,
    .thermalConductivity = 2.4f,
    .heatCapacity = 880.0f,
    .solidus = 1170.0f,
    .liquidus = 1610.0f,
    .massCohesion = 300.0f,
    .massFriction = 34.0f,
};

} // namespace derivean::sim::definition
