#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// A plate is the surface of one mantle convection cell for the length of an epoch. Its outline is
// fixed in the mantle frame; the crust riding on it moves with `velocity` and turns with `spin`.
struct Plate {
    Vec2 site;         // lattice units, centre of the cell
    double reach{};    // relative size; larger plates win more of the weighted distance
    Vec2 velocity;     // m/yr
    double spin{};     // rad/yr about the site
};

// Convection reorganises between epochs. Boundaries of one epoch are unrelated to the next, so an
// old collision belt ends up inside a later plate.
struct Epoch {
    int index{};
    double start{}, end{};   // yr
    int lattice{};           // plate sites per side of the planet
    uint64_t seed{};
    std::vector<Plate> plates;
};

// Kinematics at one point. The boundary is the nearest contact with another plate.
struct PlateSample {
    int plate{}, neighbor{};
    double boundaryDistance{};   // m to the contact
    Vec2 normal;                 // unit, from this plate towards the neighbor
    Vec2 velocity;               // m/yr, crust under the point
    Vec2 neighborVelocity;       // m/yr, crust just across the contact
    double convergence{};        // m/yr, positive when the two plates close
    double shear{};              // m/yr along the contact, signed
};

Epoch epochAt(uint64_t worldSeed, double time);

// 0 while convection reorganises at the ends of an epoch, 1 through its middle.
double epochActivity(const Epoch& epoch, double time);

PlateSample samplePlate(const Epoch& epoch, Vec2 position);

// Slow swell of the surface above upwellings and sag above downwellings, metres.
double dynamicTopography(const Epoch& epoch, Vec2 position);

} // namespace derivean::sim
