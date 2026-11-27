#pragma once

#include "planet/fn/Regions.hpp"

namespace derivean::sim {

// Each plate is one region of the mantle grid (lead, 27 Nov; planet/fn/Regions). Earth's plates follow
// Bird (2003, docs/credits): none is smaller than about 0.002 sr, and a plate cut in two by its boundaries
// is two plates. A level-5 cell is 0.0017–0.0024 sr, so a plate is also two cells at least (lead, 27 Nov):
// one cell cut off joins.
inline constexpr double MicroplateArea = 0.002;   // sr
inline constexpr int MicroplateCells = 2;

// Keeps every plate one region after a planet step. A region that is not its plate's largest and has
// MicroplateArea and MicroplateCells becomes a plate of its own: it takes the parent's Euler vector and
// motion (a new id) and its pieces keep their record, so it moves on as before and the model's forces
// drive it from there; globe.splits records it. A smaller one joins a plate round it (the one whose
// largest region it borders most, planet/fn/Cohesion.cpp), and the pieces of a plate left with no cell
// go to the plate nearest their centre; what they leave closes as a seam (closeSeams), and the closing's
// own fragments are taken in turn. A region's pieces are its plate's pieces centred in its cells or
// covering one of them most, and those that cover no cell most and are centred off their plate's cells,
// by the moving region they cover most. A plate left with no piece is dropped in the same step, the
// plates after it renumbered, and globe.plateEnds records it; stats.wholeUnsettled counts a step that
// ends with a plate other than one region with a cell.
void keepPlatesWhole(Globe& globe, int step);

} // namespace derivean::sim
