#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Shale{
    .name = "Shale",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0x596c73,
    .roughness = 0.87f,
    .density = 2300.0f,
    .compressiveStrength = 40.0f,
    .cohesion = 6000.0f,
    .frictionAngle = 22.0f,
    .slidingFriction = 0.82f,
    .loadCapacity = 210.0f,
    .stableSlope = 28.0f,
    .erodibility = 1.5e-5f,
    .abrasion = 0.30f,
    .weathering = 180.0f,
    .bedding = 1.0f,
    .jointSpacing = 0.15f,
    .weathersTo = {.blocks = 0.02f, .gravel = 0.08f, .sand = 0.05f, .silt = 0.35f, .clay = 0.50f},
    .porosity = 0.08f,
    .conductivity = 1e-11f,
    .thermalConductivity = 1.6f,
    .heatCapacity = 900.0f,
    .solidus = 1050.0f,
    .liquidus = 1450.0f,
    .massCohesion = 25.0f,
    .massFriction = 20.0f,
};

} // namespace derivean::sim::definition
