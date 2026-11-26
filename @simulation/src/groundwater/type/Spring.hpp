#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

struct World;

// Where groundwater returns to the surface. A spring is an outcome of the groundwater model: the
// water table meeting the ground, or water rising along a fault. The finest level turns its flow,
// heat and dissolved load into the forms that grow around it (sinter and travertine terraces,
// cones, pools); nothing else may place such forms.
struct Spring {
    Vec2 position;          // m
    float elevation{};      // m, where it emerges
    float discharge{};      // m³/s, mean
    float temperature{};    // °C at the vent
    float silica{};         // kg/m³ dissolved, deposits as sinter when the water cools
    float carbonate{};      // kg/m³ dissolved, deposits as travertine as the water degasses
    float steamShare{};     // 0..1 of the discharge that leaves as steam; high values are geysers and vents
    uint64_t id{};          // stable for a world: the same spring answers every query that covers it
};

// Springs whose vents lie inside the rectangle. Blocks while the levels it needs are built.
// Defined by the groundwater model; until that exists it returns none.
std::vector<Spring> springsIn(World& world, Vec2 low, Vec2 high);

} // namespace derivean::sim
