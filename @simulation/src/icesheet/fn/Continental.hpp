#pragma once

#include "icesheet/type/Continental.hpp"
#include "icesheet/type/IceGrid.hpp"

namespace derivean::sim {

struct Planet;
class Workers;

// The planet as the ice meets it: its present relief, climate and heat from below, no ice yet.
IceGrid planetIce(const Planet& planet);

// What a relaxed grid holds as one stage: grounded ice only.
PlanetStage stageOf(const IceGrid& grid, const Planet& planet);

// The stages of a world of the given age: the present grown from bare ground, then the last
// maximum and colder stages down to the coldest point of the region window, each by cooling on
// from the stage before. A sheet's hold on ground its climate would not retake from bare ground
// is therefore not in the present stage.
ContinentalIce solveContinental(const Planet& planet, double age, Workers* workers);

} // namespace derivean::sim
