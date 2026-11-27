#pragma once

#include "strata/type/Column.hpp"

#include <cstdint>

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

// A fracture of one set: sealed into a vein or not; if sealed, its mid-plane and half its widest
// thickness, in metres across the set.
struct Fracture {
    int64_t number{INT64_MIN};
    bool sealed{};
    double centre{}, half{};
};

// One vertical column of points at `material` as the fracture sets of a field meet it: where the
// column lies in each set's frame at the datum, read the first time a vein is asked for, and the
// fracture it was last asked about, which the points below share until the set's lean carries them
// into the next. Serves one seed and one field.
struct VeinColumn {
    Vec2 material;
    bool framed{};
    double along[2]{}, across[2]{};   // m in each set's frame; `across` at the datum
    Fracture last[2];
};

// cuttingRock at `depth` below the datum of the column.
Material cuttingRock(uint64_t seed, const OreField& field, VeinColumn& column, float depth, double laidDown);

// True if a vein or pipe passes within `margin` metres of the point: lets a caller skip whole columns.
bool nearCuttingRock(uint64_t seed, const OreField& field, Vec2 material, float depth, float margin);

} // namespace derivean::sim
