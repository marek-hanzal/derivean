#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

// Carbonate that spring water drops as it loses its carbon dioxide: rimmed pools and terraces.
inline constexpr MaterialDefinition Travertine{
    .name = "Travertine",
    .phase = Phase::Solid,
    .origin = Origin::Sedimentary,
    .color = 0xe2cfa4,
    .roughness = 0.80f,
    .density = 2200.0f,
    .compressiveStrength = 40.0f,
    .cohesion = 6000.0f,
    .frictionAngle = 35.0f,
    .slidingFriction = 0.70f,
    .loadCapacity = 220.0f,
    .stableSlope = 80.0f,
    .erodibility = 3.5e-6f,
    .abrasion = 0.12f,
    .weathering = 14.0f,
    .solubility = 35.0f,
    .bedding = 0.80f,
    .jointSpacing = 1.0f,
    .weathersTo = {.blocks = 0.10f, .gravel = 0.10f, .sand = 0.05f, .silt = 0.05f, .clay = 0.05f, .solute = 0.65f},
    .porosity = 0.20f,
    .conductivity = 1e-6f,
    .thermalConductivity = 1.9f,
    .heatCapacity = 840.0f,
    .solidus = 1150.0f,
    .liquidus = 1600.0f,
    .massCohesion = 80.0f,
    .massFriction = 30.0f,
};

} // namespace derivean::sim::definition
