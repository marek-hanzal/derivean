#pragma once

#include "game/type/Game.hpp"

namespace derivean {

// The bench's modes beside the flight in Flight.cpp.

// Stands for `duration` seconds at 60 frames a second, printing the readiness signals a capture
// waits for every `every` seconds and whenever one changes, with what keeps each of them false.
void watchReadiness(Game& game, double duration, double every);

// Starts a game at x and z, or with `jump` at its own spawn, puts the eye `height` metres above the
// ground at x and z in flight, and watches its readiness for `duration` seconds as watchReadiness
// does. Started there, the spawn search has already built tiles at the place; a jump arrives cold.
void watchPlace(double age, double x, double z, double height, double duration, bool jump);

// Times the steps a game's start waits for, one after another, without a game (bench/Start.cpp).
void timeStart(double age);

} // namespace derivean
