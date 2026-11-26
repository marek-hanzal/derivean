#pragma once

#include "rockmass/type/RockMass.hpp"

namespace derivean::sim {

// Bare carbonate rock opens along its joints. Rain that runs off a block into a joint widens it
// into a grike; the blocks left standing between grikes are clints. The joints are the rock mass's
// own, so a grike continues the line a coarser level eroded along. What the pavement is like
// follows in closed form from the rock, its cover and the water that runs off; how wide and deep a
// grike is along its length is seeded.
struct Pavement {
    float halfWidth{};   // m, half the width of a grike at the rock surface; 0 where the ground is no pavement
    float depth{};       // m a grike goes down
};

// `cover` is metres of loose cover on the rock, `runoff` metres a year of water left by evaporation.
Pavement pavementOf(Material rock, float cover, float runoff);

// m by which a grike lowers the rock surface at a point; 0 on a clint. `depth` is the rock
// surface's depth below the datum there, `material` the point's crust-attached position: what
// `jointDistance` is asked with. A ground cell lowers its rock by this; a voxel column is air down to it.
float grikeDepth(uint64_t seed, const Pavement& pavement, const RockMass& mass, Vec2 material, float depth);

// 0..1: share of the ground that is grike, for a level whose cells are too wide to draw one.
float grikeShare(const Pavement& pavement, const RockMass& mass);

} // namespace derivean::sim
