#pragma once

#include "Arguments.hpp"

#include "volcanic/fn/Proving.hpp"

// VolcanicScene.cpp — a vent on a plane built by hand: numbers, a map and a view.
int volcanicScene(const Arguments& arguments);
// VolcanicWorld.cpp — the vents of a place in the world and what they built there.
int volcanicWorld(const Arguments& arguments);
// VolcanicSurvey.cpp — every vent of the planet from its record alone: no tile is built.
int volcanicSurvey(const Arguments& arguments);
