#pragma once

#include "icesheet/type/IceGrid.hpp"

namespace derivean::sim {

class Workers;

// Sizes every array of the grid; no ice, no load, nothing held.
void shapeIce(IceGrid& grid, int width, int height, double cell, bool wraps);

// Surface balance and thaw of every cell for the ice it carries now.
void weighIce(IceGrid& grid, Workers* workers);

// Lets the ice flow down its own surface until what the climate adds to every cell leaves it
// again: the steady sheet of the grid's climate, reached from whatever thickness it starts with.
// Each cell takes the largest step its own stiffness allows, so the path is not a history; only
// the end state is meant. With `crustSinks` the bed gives way under the load as the sheet grows.
// Stops after `rounds`, or once no cell changes by more than `settled` metres in a round, and
// returns the largest change of the last round. Rows are worked side by side from the state of
// the round before, so the result does not depend on the pool.
float relaxIce(IceGrid& grid, Workers* workers, int rounds, float settled, bool crustSinks);

// m/yr the ice slides over its bed at a cell, along the fall of the surface: zero on a frozen bed.
struct IceSlide {
    float x{}, z{};
};
IceSlide slideAt(const IceGrid& grid, int x, int z);

// True where the cell's ice would float: it does not touch the bed.
bool afloat(const IceGrid& grid, size_t i);

} // namespace derivean::sim
