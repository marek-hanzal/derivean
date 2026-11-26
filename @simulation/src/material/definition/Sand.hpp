#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Sand{
    .name = "Sand",
    .phase = Phase::Granular,
    .origin = Origin::Unconsolidated,
    .color = 0xc3a478,
    .roughness = 0.98f,
    .density = 1600.0f,
    .frictionAngle = 33.0f,
    .slidingFriction = 0.67f,
    .loadCapacity = 22.0f,
    .stableSlope = 33.0f,
    .erodibility = 8e-5f,
    .grainSize = 5e-4f,
    .porosity = 0.35f,
    .conductivity = 1e-4f,
    .thermalConductivity = 0.3f,
    .heatCapacity = 830.0f,
    .solidus = 1450.0f,
    .liquidus = 1900.0f,
    .massCohesion = 0.0f,
    .massFriction = 33.0f,
};

} // namespace derivean::sim::definition
