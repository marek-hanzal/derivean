#pragma once

#include "measure/type/Distribution.hpp"
#include "measure/type/Mosaic.hpp"

namespace derivean::sim {

// How a finer level agrees with the level it refines, over the parent cells a child mosaic covers.
struct LevelReport {
    int parentCells{};
    Distribution drift;       // m, mean ground of the child cells in a parent cell minus the parent's ground
    double rms{};             // m
    double parentStep{};      // m, mean height step between neighbouring parent cells: the scale `drift` is read against
    int rivers{};             // dry parent cells that carry at least the river threshold
    double foundNear{};       // share of them with half their discharge in some child cell within one parent cell
    double foundWide{};       // the same within two parent cells
    double waterAgree{};      // share of parent cells whose standing water (or its absence) most of their child cells share
};

// `parent` must cover `child` completely; both are mosaics of neighbouring levels.
LevelReport levelsOf(const Mosaic& parent, const Mosaic& child, double river);

// The parent tiles under a child mosaic, as a mosaic of their own.
Mosaic gatherParents(World& world, const Mosaic& child);

} // namespace derivean::sim
