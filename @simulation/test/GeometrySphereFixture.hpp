#pragma once

#include "GeometryFixture.hpp"

#include "geometry/type/Rotation.hpp"

#include <cstdint>

namespace derivean::sim::geometry_sphere_test {

using geometry_test::expect;
using geometry_test::near;

// A reproducible uniform number in [0, 1).
struct Random {
    std::uint64_t state{};
    double next() {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return double(state >> 11) / double(1ull << 53);
    }
    Vec3 direction() {
        const double z = 2 * next() - 1, angle = 6.283185307179586 * next(), r = std::sqrt(1 - z * z);
        return {r * std::cos(angle), r * std::sin(angle), z};
    }
};

// The rotation that takes `from` to `to` along the great circle between them, then spins by `spin`
// about `to`.
Rotation carryAndSpin(Vec3 from, Vec3 to, double spin);

void checkSphereGrid();
void checkRotation();
void checkOverlap();
void checkCandidates();

} // namespace derivean::sim::geometry_sphere_test
