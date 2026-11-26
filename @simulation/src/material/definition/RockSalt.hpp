#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition RockSalt{
    .name = "RockSalt",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0xeccdc6,
    .roughness = 0.55f,
    .density = 2170.0f,
    .compressiveStrength = 25.0f,
    .cohesion = 3000.0f,
    .frictionAngle = 30.0f,
    .slidingFriction = 0.60f,
    .loadCapacity = 180.0f,
    .stableSlope = 40.0f,
    .erodibility = 2e-5f,
    .abrasion = 0.5f,
    .weathering = 50.0f,
    .solubility = 2000.0f,
    .bedding = 0.30f,
    .jointSpacing = 4.0f,
    .weathersTo = {.clay = 0.05f, .solute = 0.95f},
    .porosity = 0.01f,
    .conductivity = 1e-12f,
    .thermalConductivity = 5.5f,
    .heatCapacity = 860.0f,
    .solidus = 1074.0f,
    .liquidus = 1074.0f,
    .massCohesion = 60.0f,
    .massFriction = 28.0f,
};

} // namespace derivean::sim::definition
