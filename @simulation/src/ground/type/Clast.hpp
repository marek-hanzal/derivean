#pragma once

#include "core/type/Vec.hpp"
#include "material/constant/Materials.hpp"

#include <cstdint>

namespace derivean::sim {

// The process that left a stone where it lies.
enum class ClastOrigin : uint8_t {
    Fall,     // broke off a steep face and ran out down the slope
    Lag,      // weathered out in place and stayed while finer waste was carried off
    Stream,   // bed material of running water
    Till,     // dropped by ice, with the rock the ice passed over
    Frost,    // heaved and sorted by freezing ground
};

inline constexpr int ClastOrigins = 5;

// One stone larger than a voxel: a piece of a particular bed, with the shape its journey gave it.
// Positions are metres from the origin of the ground tile that owns it, so single precision holds
// them to a fraction of a millimetre anywhere on the planet.
struct Clast {
    float x{}, z{};           // m east and south of the owning tile's corner
    float y{};                // m, elevation of the centre
    float a{}, b{}, c{};      // m, half-axes: longest, intermediate, shortest (the shortest stands up)
    float yaw{}, pitch{}, roll{};   // rad: heading of the long axis, its dip, and the lean of the flat side about it
    float roundness{};        // 0 fresh and angular … 1 worn round
    float travel{};           // m carried from the source
    Vec2 source;              // m, world position of the rock it broke from
    Material rock{Material::Bedrock};
    ClastOrigin origin{ClastOrigin::Lag};

    // A fresh block is nearly a box, a worn one an ellipsoid: |x/a|^n + |y/c|^n + |z/b|^n <= 1.
    int exponent() const { return roundness > 0.6f ? 2 : roundness > 0.25f ? 4 : 6; }
    float volume() const { return (roundness > 0.6f ? 4.18879f : roundness > 0.25f ? 6.4818f : 7.2084f) * a * b * c; }
    float reach() const { return 1.75f * a; }   // m from the centre that no part of it exceeds
};

inline constexpr float SmallestClast = 0.11f;   // m, half of the intermediate axis: below this a stone is matrix
inline constexpr float LargestClast = 2.4f;     // m, half of the longest axis
inline constexpr double ClastReach = 1.75 * LargestClast;   // m a stone can reach beyond the tile that owns it

} // namespace derivean::sim
