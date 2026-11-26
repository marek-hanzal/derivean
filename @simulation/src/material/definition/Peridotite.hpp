#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Peridotite{
    .name = "Peridotite",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x4f5a34,
    .roughness = 0.82f,
    .density = 3300.0f,
    .compressiveStrength = 200.0f,
    .cohesion = 40000.0f,
    .frictionAngle = 50.0f,
    .slidingFriction = 0.82f,
    .loadCapacity = 320.0f,
    .stableSlope = 60.0f,
    .erodibility = 1.4e-6f,
    .abrasion = 0.01f,
    .weathering = 30.0f,
    .bedding = 0.05f,
    .jointSpacing = 2.0f,
    .weathersTo = {.blocks = 0.25f, .gravel = 0.20f, .sand = 0.15f, .silt = 0.10f, .clay = 0.30f},
    .porosity = 0.005f,
    .conductivity = 1e-10f,
    .thermalConductivity = 4.0f,
    .heatCapacity = 820.0f,
    .solidus = 1400.0f,
    .liquidus = 2000.0f,
    .massCohesion = 250.0f,
    .massFriction = 32.0f,
};

} // namespace derivean::sim::definition
