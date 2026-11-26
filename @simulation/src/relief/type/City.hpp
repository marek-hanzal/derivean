#pragma once

#include "rockmass/type/RockMass.hpp"

namespace derivean::sim {

// Massive rock near a cliff band opens along the planes of its rock mass into slots with sand on
// their floors and free-standing towers between them; a fin between two slots can be cut through
// into an arch. The planes are the rock mass's own, of the first order that stands far enough
// apart to leave walls between the streets, so a slot continues the gully a coarser level cut
// along the same plane. How wide and deep a street has opened along its length is seeded.
struct RockCity {
    uint64_t seed{};           // of the world: the planes are asked with it
    float strength{};          // 0..1 how far the rock has opened here; under 0.05 there is no city
    float floor{};             // m, elevation the slots go down to: the base of the massive rock
    RockMass mass;
    int order[JointSets]{};    // per set, the order whose planes open; JointOrders for none
};

inline constexpr float StreetSpacing = 6.0f;   // m between planes below which walls would not stand between slots

// Sets `order` from the spacings of `mass`.
void chooseStreets(RockCity& city);

// True where rock at a point of a column whose rock surface is `rockTop` has been opened. `depth`
// is metres below the datum; `sand` is set where the point lies on the floor of a slot.
bool cityOpens(const RockCity& city, Vec2 material, float y, float depth, float rockTop, bool& sand);

} // namespace derivean::sim
