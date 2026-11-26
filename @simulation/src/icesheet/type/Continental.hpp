#pragma once

#include "core/type/Field.hpp"

#include <vector>

namespace derivean::sim {

// The grounded ice of the whole planet in the steady state of one climate, on the planet grid.
struct IceStage {
    double cooling{};           // °C against the present, zero or negative
    Field<float> thickness;     // m; zero where there is none or it floats
    Field<float> sink;          // m the load has pressed the crust down
    double volume{};            // m³ of grounded ice
    double landShare{};         // share of the present land it covers
};

// Stages a world's glaciations pass through, warmest first: the present, the last glacial maximum,
// and as many colder ones as the maxima of the replayed window need. A date is described by the
// time its cycles spend near each stage (icesheet/fn/Dating.hpp), so a region never asks for a
// sheet between two tiles: every tile reads the same planet-wide stages.
struct ContinentalIce {
    std::vector<IceStage> stages;
    Field<float> heat;          // K/m, temperature gradient in the rock, per planet cell
    double present{};           // yr, the end of the last termination: the date stage 0 stands for
};

inline constexpr int IceNow = 0;
inline constexpr int IceLastMaximum = 1;

} // namespace derivean::sim
