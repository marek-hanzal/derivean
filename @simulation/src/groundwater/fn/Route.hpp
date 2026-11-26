#pragma once

#include "groundwater/type/Aquifer.hpp"

namespace derivean::sim {

// What a node does with the water that reaches it.
struct NodeSplit {
    float down{};       // m³/yr through the tight bed into the storey below
    float side{};       // m³/yr on to its neighbours
    float out{};        // m³/yr out at the surface of the cell
    float lost{};       // m³/yr through the base of circulation
    float captured{};   // m³/yr into the conduits of soluble rock
};
NodeSplit splitNode(const WaterField& field, size_t cell, int storey, float flux);

// m below the ground that water passing a node goes. `high` is the elevation its catchment stands at.
float flowDepth(const WaterField& field, size_t cell, int storey, float high);

} // namespace derivean::sim
