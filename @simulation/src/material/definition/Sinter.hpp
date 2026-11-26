#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

// Opaline silica that hot spring water lays down as it cools: rims, terraces and geyser cones.
inline constexpr MaterialDefinition Sinter{
    .name = "Sinter",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0xe4e0d6,
    .roughness = 0.70f,
    .density = 1900.0f,
    .compressiveStrength = 25.0f,
    .cohesion = 4000.0f,
    .frictionAngle = 34.0f,
    .slidingFriction = 0.62f,
    .loadCapacity = 200.0f,
    .stableSlope = 80.0f,
    .erodibility = 4e-6f,
    .abrasion = 0.10f,
    .weathering = 20.0f,
    .solubility = 2.0f,
    .bedding = 0.85f,
    .jointSpacing = 0.5f,
    .weathersTo = {.blocks = 0.10f, .gravel = 0.30f, .sand = 0.40f, .silt = 0.15f, .solute = 0.05f},
    .porosity = 0.25f,
    .conductivity = 1e-6f,
    .thermalConductivity = 1.1f,
    .heatCapacity = 780.0f,
    .solidus = 1400.0f,
    .liquidus = 1900.0f,
    .massCohesion = 50.0f,
    .massFriction = 28.0f,
};

} // namespace derivean::sim::definition
