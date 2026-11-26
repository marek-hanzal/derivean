#pragma once

#include "simulation/type/World.hpp"

namespace derivean::sim {

// Places worth measuring, found from what the world itself says rather than from fixed coordinates.

// Highest point of the planet within `reach` of a point, as a landscape built there will see it.
Vec2 highestNear(const Planet& planet, Vec2 near, double reach);

// Centre of the dry Region cell nearest to `near` that has the sea on one of its four sides, in
// the 3 × 3 Region tiles around it; `near` itself if they hold no coast. Builds those tiles.
Vec2 coastNear(World& world, Vec2 near);

} // namespace derivean::sim
