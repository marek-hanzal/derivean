#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

// The deposit of a hot pumice-and-ash current whose shards welded as it settled: it stands in cliffs and sets columns.
inline constexpr MaterialDefinition Ignimbrite{
    .name = "Ignimbrite",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0xb08a76,
    .roughness = 0.92f,
    .density = 2300.0f,
    .compressiveStrength = 80.0f,
    .cohesion = 14000.0f,
    .frictionAngle = 40.0f,
    .slidingFriction = 0.78f,
    .loadCapacity = 230.0f,
    .stableSlope = 70.0f,
    .erodibility = 3e-6f,
    .abrasion = 0.03f,
    .weathering = 30.0f,
    .bedding = 0.40f,
    .jointSpacing = 1.5f,
    .weathersTo = {.blocks = 0.25f, .gravel = 0.25f, .sand = 0.25f, .silt = 0.15f, .clay = 0.10f},
    .porosity = 0.10f,
    .conductivity = 3e-7f,
    .thermalConductivity = 1.6f,
    .heatCapacity = 850.0f,
    .solidus = 1050.0f,
    .liquidus = 1300.0f,
    .massCohesion = 150.0f,
    .massFriction = 32.0f,
};

} // namespace derivean::sim::definition
