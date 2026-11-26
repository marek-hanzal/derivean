#pragma once

#include "ground/type/SpringGround.hpp"
#include "landscape/type/Level.hpp"

namespace derivean::sim {

struct SpringBounds {
    Vec2 low, high;
};

inline SpringBounds groundSpringBounds(TileKey ground) {
    const double span = tileSpan(ground.level);
    const Vec2 corner{ground.x * span, ground.z * span};
    return {{corner.x - SpringReach, corner.z - SpringReach},
            {corner.x + span + SpringReach, corner.z + span + SpringReach}};
}

} // namespace derivean::sim
