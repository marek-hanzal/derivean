#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// A square of ground around a spring and what the spring has built on it. Row-major, `side²` cells.
struct SpringGround {
    Vec2 origin;                  // world position of the outer corner of cell (0, 0)
    double cell{};
    int side{};
    std::vector<float> ground;    // m, the surface the water first ran over
    std::vector<float> crust;     // m of mineral laid on it
    std::vector<uint8_t> kind;    // Crust
    std::vector<float> altered;   // 0..1, how far hot acid water has turned the ground to clay
    std::vector<float> pool;      // m, depth of water standing on the deposit; 0 where it only runs or is dry
};

inline constexpr double SpringReach = 24.0;   // m around a vent within which its deposit is followed

} // namespace derivean::sim
