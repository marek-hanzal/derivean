#pragma once

#include "measure/fn/Orientation.hpp"
#include "measure/type/Distribution.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// Plain RGB pictures for diagnostics/fn/Png.hpp; no labels, the numbers are printed next to them.

// Petals by 5° of valley azimuth, mirrored through the centre. The red ring is where every petal of
// an isotropic network ends; grey lines mark the grid axes and diagonals. East is right, south down.
std::vector<uint8_t> rosePlot(const Rose& rose, int size);

// Bars from the bottom edge, the tallest filling the height.
std::vector<uint8_t> barPlot(const Histogram& histogram, int width, int height);

} // namespace derivean::sim
