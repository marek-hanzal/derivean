#pragma once

#include "material/type/MaterialDefinition.hpp"

namespace derivean::sim::definition {

inline constexpr MaterialDefinition Leaf{
    .name = "Leaf",
    .phase = Phase::Solid,
    .origin = Origin::Organic,
    .color = 0x9cae62,
    .roughness = 0.90f,
    .density = 100.0f,
    .slidingFriction = 0.55f,
    .loadCapacity = 0.35f,
    .porosity = 0.88f,
    .conductivity = 1e-2f,
    .thermalConductivity = 0.05f,
    .heatCapacity = 2800.0f,
};

} // namespace derivean::sim::definition
