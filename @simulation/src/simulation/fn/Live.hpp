#pragma once

#include "intervention/type/Overlay.hpp"
#include "relief/type/Ground.hpp"
#include "simulation/fn/Query.hpp"

namespace derivean::sim {

// What the session changed in the site cell a point lies in: the live column against the untouched
// one. The untouched answers keep their own shape, finer than a site cell, and are moved by this.
struct CellChange {
    const EditedTile* tile{};                    // null where the world is untouched there
    size_t at{};                                 // the cell in the tile's arrays
    float rock{}, cover[SedimentClasses]{};      // m added to the rock and to each class of cover

    float ground() const { return rock + cover[0] + cover[1] + cover[2]; }
};

CellChange changeAt(const World& world, double x, double z);

// Moves every node of a ground lattice by the change of its site cell. True if any node moved.
bool changeLattice(const World& world, GroundLattice& lattice);

// Builds every tile of a level that a rectangle grown by `margin` touches.
void requireOver(World& world, Level level, double minX, double minZ, double maxX, double maxZ, double margin);

} // namespace derivean::sim
