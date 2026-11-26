#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Chalk{
    .name = "Chalk",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0xc9ad84,
    .roughness = 0.83f,
    .density = 2100.0f,
    .compressiveStrength = 10.0f,
    .cohesion = 2000.0f,
    .frictionAngle = 32.0f,
    .slidingFriction = 0.75f,
    .loadCapacity = 220.0f,
    .stableSlope = 55.0f,
    .erodibility = 6e-6f,
    .abrasion = 0.20f,
    .weathering = 40.0f,
    .solubility = 40.0f,
    .bedding = 0.40f,
    .jointSpacing = 1.0f,
    .weathersTo = {.blocks = 0.05f, .gravel = 0.05f, .sand = 0.02f, .silt = 0.10f, .clay = 0.08f, .solute = 0.70f},
    .porosity = 0.38f,
    .conductivity = 1e-8f,
    .thermalConductivity = 1.2f,
    .heatCapacity = 900.0f,
    .solidus = 1170.0f,
    .liquidus = 1610.0f,
    .massCohesion = 60.0f,
    .massFriction = 30.0f,
};

} // namespace derivean::sim::definition
