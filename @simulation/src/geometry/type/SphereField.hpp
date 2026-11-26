#pragma once

#include <vector>

namespace derivean::sim {

// Values on the cells of one level of the sphere grid, one per cell in the grid's order
// (face · side + j) · side + i (fn/SphereGrid.hpp: indexOf, cellAt; sphereCellCount gives the size).
// The level travels with the values, so a field cannot be read with another level's index or sampled
// as a flat wrapped raster.
template <class T>
struct SphereField {
    int level{};
    std::vector<T> value;
};

} // namespace derivean::sim
