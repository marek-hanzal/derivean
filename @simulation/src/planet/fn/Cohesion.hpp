#pragma once

#include "planet/fn/Regions.hpp"

namespace derivean::sim {

class Workers;

// Keeps every plate one region after a planet step. A region that is not its plate's largest and has
// MicroplateArea and MicroplateCells becomes a plate of its own: it takes the parent's Euler vector and
// motion (a new id) and its pieces keep their record, so it moves on as before and the model's forces
// drive it from there; globe.splits records it. A smaller one joins a plate round it (the one whose
// largest region shares its longest border, in metres, planet/fn/Cohesion.cpp), and the pieces of a
// plate left with no cell go to the plate nearest their centre; what they leave closes as a seam
// (closeSeams), and the closing's own fragments are taken in turn. A region's pieces are its plate's
// pieces centred in its cells or covering one of them most, and those that cover no owned cell most and
// are centred off their plate's cells, by the moving region they cover most. A plate left with no
// piece is dropped in the same step, the plates after it renumbered, and globe.plateEnds records it;
// stats.wholeUnsettled counts a step that ends with a plate other than one region with a cell: one that
// another plate's cells cut in two, or one that holds pieces and no cell.
void keepPlatesWhole(Globe& globe, int step, Workers* workers = nullptr);

} // namespace derivean::sim
