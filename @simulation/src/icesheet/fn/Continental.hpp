#pragma once

#include "icesheet/type/Continental.hpp"
#include "icesheet/type/IceGrid.hpp"

namespace derivean::sim {

struct Planet;
class Workers;

// The planet as the ice meets it: its present relief, climate and heat from below, no ice yet.
IceGrid planetIce(const Planet& planet);

// What a relaxed grid holds as one stage: grounded ice only.
IceStage stageOf(const IceGrid& grid, const Planet& planet);

// The stages of a world of the given age: the last maximum grown from bare ground, the present
// reached by warming from it (what is left of that sheet, not what bare ground would grow), and
// colder stages by cooling on from the last maximum down to the coldest point of the region window.
ContinentalIce solveContinental(const Planet& planet, double age, Workers* workers);

} // namespace derivean::sim
