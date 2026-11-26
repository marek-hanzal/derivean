#pragma once

#include "chart/type/Chart.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

// Finest level a chart of this pitch reads, never finer than Site; false when a pixel is too wide
// even for a region.
bool chartFinest(double pitch, Level& finest);

// Reads the square from what is finished: each pixel from the finest tile level built under its
// centre that the pitch can show (the answer of surfaceAt there), and from the planet where no tile
// is. Never blocks and never starts a build, so the same request gets sharper as tiles arrive.
// Water is drawn to be read at any pitch. `waters` and `water` are surfaceAt's at the pixel centre,
// with two exceptions where pixels are narrower than cells: only the path of a cell's stream from
// cell centre to cell centre is a stream, not the whole cell, and water standing next door reaches
// in where the pixel's own ground lies below it. `discharge` is the largest stream a pixel covers
// or lies on.
// It only reads the planet and finished tiles, neither of which changes once it exists: any number
// of threads may read charts while tiles are being built.
Chart chartOf(const World& world, const ChartRequest& request);

} // namespace derivean::sim
