#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

struct World;

// Where groundwater returns to the surface. A spring is an outcome of the groundwater model: the
// water table meeting the ground, a tight bed turning water out where it crops out, water rising
// along a fault, or the outlet of a conduit in soluble rock. The finest level turns its flow,
// heat and dissolved load into the forms that grow around it (sinter and travertine terraces,
// cones, pools); nothing else may place such forms.
struct Spring {
    Vec2 position;          // m
    float elevation{};      // m, where it emerges
    float discharge{};      // m³/s, mean
    float temperature{};    // °C at the vent
    // What the water gives up at the surface, not all it holds: silica beyond what cooled water
    // keeps, carbonate beyond what stays dissolved once the gas that held it has gone to the air.
    // Cold water that only met the air's carbon dioxide brings neither. SpringRecord has the totals.
    float silica{};         // kg/m³, comes out as sinter when the water cools
    float carbonate{};      // kg/m³, comes out as travertine as the water degasses
    float steamShare{};     // 0..1 of the discharge that leaves as steam; high values are geysers and vents
    uint64_t id{};          // stable for a world: the same spring answers every query that covers it
};

// Springs whose vents lie inside the rectangle, low edge included. Blocks while the ground water
// of the local tiles under it is worked out and the site tiles under the vents are built: a vent is
// found at the local level and put on the site level's ground (groundwater/fn/Refine.hpp).
std::vector<Spring> springsIn(World& world, Vec2 low, Vec2 high);

} // namespace derivean::sim
