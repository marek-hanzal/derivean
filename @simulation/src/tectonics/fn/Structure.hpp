#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>

namespace derivean::sim {

// How deformation that the planet only knows as a rate per 65 km cell is distributed inside it.
// The patterns are seeded rock-quality structure attached to the crust (material coordinates);
// each averages to zero, so they move the supplied thickening around without adding any.
struct Structure {
    double ranges{};   // >= -1: share of stacking taken by individual thrust ranges
    double rifts{};    // >= -1: share of stretching taken by individual grabens
    double blocks{};   // [-1, 1]: tilted fault blocks, steep on one side and gentle on the other
    double weakness{}; // [0, 1]: crushed rock along faults and joints, where water cuts fastest
};

Structure structureAt(uint64_t seed, Vec2 material, double strike);

} // namespace derivean::sim
