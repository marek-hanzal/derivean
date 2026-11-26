#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

// Cinders and pumice the size of peas to walnuts that fell around a vent and were bonded by later burial.
inline constexpr MaterialDefinition LapilliTuff{
    .name = "LapilliTuff",
    .phase = Phase::Solid,
    .origin = Origin::Igneous,
    .color = 0x7a5548,
    .roughness = 0.98f,
    .density = 1500.0f,
    .compressiveStrength = 8.0f,
    .cohesion = 1200.0f,
    .frictionAngle = 36.0f,
    .slidingFriction = 0.75f,
    .loadCapacity = 140.0f,
    .stableSlope = 50.0f,
    .erodibility = 9e-6f,   // Tuff's: weaker in the hand, but its clasts need more stream power than ash to carry
    .abrasion = 0.15f,
    .weathering = 70.0f,
    .bedding = 0.60f,
    .jointSpacing = 2.0f,
    .weathersTo = {.blocks = 0.02f, .gravel = 0.40f, .sand = 0.35f, .silt = 0.13f, .clay = 0.10f},
    .porosity = 0.40f,
    .conductivity = 1e-5f,
    .thermalConductivity = 0.6f,
    .heatCapacity = 900.0f,
    .solidus = 1050.0f,
    .liquidus = 1300.0f,
    .massCohesion = 40.0f,
    .massFriction = 33.0f,
};

} // namespace derivean::sim::definition
