#pragma once

#include "core/type/Field.hpp"

namespace derivean::sim {

struct Planet;
class Workers;

// Annual-mean state of the planet's moisture on the planet grid.
struct MoistureBalance {
    Field<float> vapour;          // kg/m² in the column
    Field<float> evaporation;     // kg/m²/s from open water and wet ground
    Field<float> precipitation;   // kg/m²/s
    double evaporated{};          // kg/s over the planet
    double precipitated{};        // kg/s over the planet
};

// Steady state of the moisture budget the stepped atmosphere integrates in time, with the same
// laws and constants: vapour evaporates from open water and from ground that rain keeps wet, the
// zonal-mean belts carry it, weather systems spread it like a diffusion, and it condenses where
// the column holds more than the air over that ground can — on rising terrain first, which leaves
// the lee dry.
//
// Closure: the budget is in flux form on a closed surface, so in its steady state the
// precipitation summed over the planet equals the evaporation summed over the planet. The solver
// iterates towards that state; the two totals it returns agree to a fraction of a percent. Land
// returns to the air what the lesser of its rain and the air's thirst allows and the rest runs
// off to the sea, so without open water there is no source and no precipitation.
MoistureBalance solveMoisture(const Planet& planet, Workers& workers);

} // namespace derivean::sim
