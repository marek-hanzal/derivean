#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Conglomerate{
    .name = "Conglomerate",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0x7c594b,
    .roughness = 0.93f,
    .density = 2450.0f,
    .compressiveStrength = 70.0f,
    .cohesion = 12000.0f,
    .frictionAngle = 38.0f,
    .slidingFriction = 0.82f,
    .loadCapacity = 240.0f,
    .stableSlope = 70.0f,
    .erodibility = 2.5e-6f,
    .abrasion = 0.02f,
    .weathering = 30.0f,
    .bedding = 0.40f,
    .jointSpacing = 5.0f,
    .weathersTo = {.blocks = 0.15f, .gravel = 0.50f, .sand = 0.30f, .silt = 0.03f, .clay = 0.02f},
    .porosity = 0.12f,
    .conductivity = 5e-6f,
    .thermalConductivity = 2.4f,
    .heatCapacity = 840.0f,
    .solidus = 1300.0f,
    .liquidus = 1700.0f,
    .massCohesion = 250.0f,
    .massFriction = 33.0f,
};

} // namespace derivean::sim::definition
