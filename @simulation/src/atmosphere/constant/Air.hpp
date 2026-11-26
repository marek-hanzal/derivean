#pragma once

#include "planet/constant/Scale.hpp"

namespace derivean::sim {

// The weather grid covers the planet with cells of 2×2 planet cells and steps on absolute
// multiples of AirStep in calendar time, so reaching a date in one call or in many gives one state.
// Within a step the wind and what it carries take WindSteps shorter steps, because air crosses a
// cell faster than heat and water change in it.
inline constexpr int AirCells = 128;
inline constexpr double AirCell = PlanetSpan / AirCells;   // m
inline constexpr double AirCellArea = AirCell * AirCell;
inline constexpr double AirStep = 3600.0;                  // s
inline constexpr int AirStepsPerDay = 24;
inline constexpr int WindSteps = 2;
inline constexpr double WindStep = AirStep / WindSteps;    // s
inline constexpr int AirSpinUp = 20 * AirStepsPerDay;      // steps run before the start date

// The world's substitute for the Coriolis parameter at a pole. The planet is 0.42 of a full-size
// one, so its rotation is slowed by the same share to keep the change of planetary vorticity with
// latitude, which is what steers weather systems, at the familiar strength.
inline constexpr double Coriolis = 6.0e-5;                 // 1/s

} // namespace derivean::sim
