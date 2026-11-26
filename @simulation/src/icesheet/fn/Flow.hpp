#pragma once

#include "icesheet/type/IceGrid.hpp"

namespace derivean::sim {

class Workers;

// Sizes every array of the grid; no ice, no load, nothing held.
void shapeIce(IceGrid& grid, int width, int height, double cell, bool wraps);

// Surface balance and thaw of every cell for the ice it carries now, read afresh.
void weighIce(IceGrid& grid, Workers* workers);

// Lets the ice flow down its own surface until what the climate adds to every cell leaves it
// again: the steady sheet of the grid's climate, reached from whatever thickness it starts with.
// Each cell takes the largest step its own stiffness allows, so the path is not a history; only
// the end state is meant. With `crustSinks` the bed gives way under the load as the sheet grows.
// Stops after `most` rounds, or once the volume of ice holds still to the share `settled` over
// forty rounds, and returns the rounds taken. Rows are worked side by side from the state of the
// round before, so neither the result nor the count depends on the pool.
int relaxIce(IceGrid& grid, Workers* workers, int most, float settled, bool crustSinks);

// m/yr the ice slides over its bed at a cell, along the fall of the surface: zero on a frozen bed.
struct IceSlide {
    float x{}, z{};
};
IceSlide slideAt(const IceGrid& grid, int x, int z);

// True where the cell's ice would float: it does not touch the bed.
bool afloat(const IceGrid& grid, size_t i);

} // namespace derivean::sim
