#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Magma{
    .name = "Magma",
    .phase = Phase::Liquid,
    .origin = Origin::Fluid,
    .color = 0xff6a1a,
    .roughness = 0.5f,
    .density = 2700.0f,
    .thermalConductivity = 1.5f,
    .heatCapacity = 1450.0f,
    .solidus = 1250.0f,
    .liquidus = 1500.0f,
    .dynamicViscosity = 100.0f,
    .surfaceTension = 0.36f,
};

} // namespace derivean::sim::definition
