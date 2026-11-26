#pragma once

#include "volcanic/type/Unrest.hpp"

namespace derivean::sim {

struct World;

// Unrest.cpp — a vent at one moment, as its chamber stands: a function of the seed, the vent and
// the date alone, so asking twice, walking there or jumping a century gives the same answer.
// `time` in years of the era: the world's age plus what the calendar has added.
Unrest ventUnrest(uint64_t seed, const Vent& vent, double time);
// m of that uplift at a distance from the vent: a small source swelling at the chamber's depth.
double upliftAt(const Unrest& unrest, double distance);

// What stands at a place and why. Builds the region tile there and blocks until it is finished.
VolcanicRecord volcanicAt(World& world, Vec2 position, double calendarYears, double reach = 60000.0);

} // namespace derivean::sim
