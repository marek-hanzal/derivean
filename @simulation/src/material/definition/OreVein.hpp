#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition OreVein{
    .name = "OreVein",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0xe8d896,
    .roughness = 0.60f,
    .density = 2850.0f,
    .compressiveStrength = 180.0f,
    .cohesion = 40000.0f,
    .frictionAngle = 50.0f,
    .slidingFriction = 0.76f,
    .loadCapacity = 280.0f,
    .stableSlope = 70.0f,
    .erodibility = 9e-7f,
    .abrasion = 0.004f,
    .weathering = 8.0f,
    .bedding = 0.10f,
    .jointSpacing = 1.0f,
    .weathersTo = {.blocks = 0.45f, .gravel = 0.30f, .sand = 0.20f, .silt = 0.03f, .clay = 0.02f},
    .porosity = 0.01f,
    .conductivity = 1e-9f,
    .thermalConductivity = 5.0f,
    .heatCapacity = 720.0f,
    .solidus = 1300.0f,
    .liquidus = 1950.0f,
    .massCohesion = 300.0f,
    .massFriction = 34.0f,
};

} // namespace derivean::sim::definition
