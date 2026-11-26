#pragma once

#include "ground/type/Clast.hpp"
#include "ground/type/CubeLattice.hpp"
#include "landscape/type/Terrain.hpp"

#include <functional>

namespace derivean::sim {

// Writes the rock of every stone of a ground tile over the cube centres it contains. A stone can
// reach ClastReach beyond the tile that owns it, so a reader passes every tile within that of its
// lattice. `lift`, where given, is the metres the ground under a world position has been moved
// since the tile was made: a stone goes with the ground it lies on.
void stampClasts(const Tile& tile, const CubeLattice& lattice, Material* materials, const std::function<float(double x, double z)>* lift = nullptr);

// The stone of a ground tile that a world point lies in, or null. `lift` as for stampClasts.
const Clast* clastAt(const Tile& tile, double x, double y, double z, const std::function<float(double x, double z)>* lift = nullptr);

} // namespace derivean::sim
