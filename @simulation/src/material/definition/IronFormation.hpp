#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition IronFormation{
    .name = "IronFormation",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0x70382e,
    .roughness = 0.72f,
    .density = 3400.0f,
    .compressiveStrength = 250.0f,
    .cohesion = 50000.0f,
    .frictionAngle = 45.0f,
    .slidingFriction = 0.78f,
    .loadCapacity = 300.0f,
    .stableSlope = 70.0f,
    .erodibility = 8e-7f,
    .abrasion = 0.003f,
    .weathering = 8.0f,
    .bedding = 0.80f,
    .jointSpacing = 1.0f,
    .weathersTo = {.blocks = 0.45f, .gravel = 0.30f, .sand = 0.15f, .silt = 0.05f, .clay = 0.05f},
    .porosity = 0.02f,
    .conductivity = 1e-9f,
    .thermalConductivity = 4.5f,
    .heatCapacity = 700.0f,
    .solidus = 1420.0f,
    .liquidus = 1850.0f,
    .massCohesion = 350.0f,
    .massFriction = 34.0f,
};

} // namespace derivean::sim::definition
