#pragma once

#include "game/type/Game.hpp"

namespace derivean {

// Stands for `duration` seconds at 60 frames a second, printing the readiness signals a capture
// waits for every `every` seconds and whenever one changes, with what keeps each of them false.
void watchReadiness(Game& game, double duration, double every);

// Times the steps a game's start waits for, one after another, without a game (bench/Start.cpp).
void timeStart(double age);

} // namespace derivean
