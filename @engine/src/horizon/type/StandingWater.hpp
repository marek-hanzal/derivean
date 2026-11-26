#pragma once

#include "simulation/type/World.hpp"

#include <functional>
#include <vector>

namespace derivean {

// Points (i, j) at (x + i · cell, z + j · cell) metres for i, j in [0, side).
struct WaterLattice {
    double x{}, z{}, cell{};
    int side{};
};

inline constexpr int WaterRing = 6;        // points around a lattice that its water is read from
inline constexpr float WetDepth = 0.03f;   // m of water over the ground below which it is a film, not a water body

// The level of standing water at every point of a lattice in metres, NaN where there is none.
// Standing water is the flood level of a lake or of the sea, one value per water body:
//  - it is read from the site level, or the finest coarser one finished there, whatever the
//    lattice, so everything that touches a lake shows the same lake;
//  - a wet point belongs to a water body only if the point one data cell away is wet at the same
//    level. A stream's depth on a slope and a one-cell pit of coarse data have no such partner and
//    get no sheet;
//  - the data marks water per cell while the ground runs smoothly through it, so the level is
//    carried on across ground that lies below it, a data cell at most, and one point into the dry
//    shore. The sheet then meets the ground at its depth-zero contour, not at a cell edge.
// `ground(i, j)` is the caller's own ground at a point and answers WaterRing points beyond the
// lattice to every side. Two lattices of one cell size that share a point agree on it, as long as
// their callers agree on the ground.
std::vector<float> standingWater(const sim::World& world, WaterLattice lattice, const std::function<float(int, int)>& ground);

} // namespace derivean
