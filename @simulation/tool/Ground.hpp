#pragma once

#include "Arguments.hpp"

#include "simulation/type/World.hpp"

// SliceCommand.cpp — one place as a plan and two sections of the voxel answer, written as
// `<out>/<name>-plan.png`, `-section.png` and `-section-ns.png`; sizes come from the arguments.
void renderSlice(derivean::sim::World& world, const Arguments& arguments, double x, double z, const std::string& name);

// GroundFind.cpp — places of each kind of ground, for looking at the finest level where it matters.
int groundFind(const Arguments& arguments);

// GroundMap.cpp — the stored ground tiles under a square as pictures, one pixel per cell.
int groundMap(const Arguments& arguments);

// GroundCommand.cpp — statistics of the ground level over a square of `span` metres: stones, cover,
// spacings, the coarse layer's budget, and what the ground cost to make and to read.
int groundReport(const Arguments& arguments);
void reportGround(derivean::sim::World& world, derivean::sim::Vec2 centre, double span);
