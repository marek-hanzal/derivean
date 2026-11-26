#pragma once

#include "strata/type/Column.hpp"

namespace derivean::sim {

// Bodies that cut the layering instead of following it: veins that hot fluids sealed into the
// fractures around an intrusion, and pipes that deep melt punched through old crust. They are fixed
// in the crust, so a place keeps them as erosion strips the beds they cut. Where they are follows
// from the province the planet's history left (kind of crust, melt it received, its plutons); which
// fracture of a veined ground was sealed is seeded.
struct OreField {
    float veining{};        // 0..1 share of the fracture sets that fluids sealed; 0 for most ground
    double strike{};        // rad, trend of the main set
    double emplaced{};      // yr the fluids moved; only rock older than this is cut
    Vec2 pipe;              // crust-attached centre of the pipe that reaches this ground
    float pipeRadius{};     // m at the datum; 0 where there is none
    double pipeEmplaced{};  // yr
    uint64_t pipeId{};      // stable for a world
};

// Evaluate once per column or coarser: it is the costly half.
OreField oreFieldAt(uint64_t seed, const StrataColumn& column, Vec2 material);

// The cutting rock at a point `depth` below the datum, or Air where the beds are intact. Cheap where
// the point is clear of every vein. `laidDown` is the age of the unit at the point (0 for basement):
// a body does not cut rock that was laid over it later.
Material cuttingRock(uint64_t seed, const OreField& field, Vec2 material, float depth, double laidDown);

// True if a vein or pipe passes within `margin` metres of the point: lets a caller skip whole columns.
bool nearCuttingRock(uint64_t seed, const OreField& field, Vec2 material, float depth, float margin);

} // namespace derivean::sim
