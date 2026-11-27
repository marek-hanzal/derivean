#pragma once

#include "geometry/type/Vec3.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// Plates on the sphere move as rigid rotations about Euler poles (McKenzie & Parker 1967; Morgan
// 1968). A plate of an epoch is the surface of one mantle convection cell: a weighted spherical
// Voronoi region of its site, its contacts bent by 3D noise. Unlike the flat Plate it carries no
// velocity of its own: the crust pieces it holds (planet/type/Globe.hpp) turn about `pole`.
struct EulerPlate {
    Vec3 site;          // unit
    double reach{};     // relative size; larger plates win more of the weighted angular distance
    Vec3 pole;          // unit, right-handed axis of the rotation
    double speed{};     // m/yr, the RMS speed the plate is given over its own crust
    double draw{};      // the uniform factor of that speed, in [0.7, 1.3) for Earth's speeds
    Vec3 euler;         // rad/yr, undamped: pole · speed / (R · sqrt(<sin² γ>))
};

// Which speeds plates get: today's flat history (4 to 30 mm/yr, uniform) to compare with it, or
// Earth's (no-net-rotation frame, Argus, Gordon & DeMets 2011; slab-pulled oceanic plates fast,
// continental ones slow, Forsyth & Uyeda 1975).
enum class PlateSpeeds : uint8_t { Today, Earth };

// One epoch on the sphere. Its index, times and seed are those of the flat epochAt for the same
// world, so the two histories reorganise at the same moments.
struct EulerEpoch {
    int index{};
    double start{}, end{};   // yr
    uint64_t seed{};
    std::vector<EulerPlate> plates;   // the epoch's sites, then the plates split from them within it (planet/fn/Cohesion)
    size_t sites{};                   // plates the epoch began with and still has, first in `plates`; 0: all of them
};

// Σ a (I − x xᵀ) over a set of crust pieces (areas a, unit centres x): the moment that turns a rotation
// vector into the net rotation it gives the surface. Symmetric, so six numbers.
struct Inertia {
    double xx{}, xy{}, xz{}, yy{}, yz{}, zz{};
};

} // namespace derivean::sim
