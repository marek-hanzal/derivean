#pragma once

#include "chart/type/Chart.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

// Finest level a chart of this pitch reads, down to Ground (what the voxels are cut from, so a chart
// fine enough agrees with what the player stands on); false when a pixel is too wide even for a region.
bool chartFinest(double pitch, Level& finest);

// Reads the square from what is finished: each pixel from the finest tile level built under its
// centre that the pitch can show (what readSurface gives there), and from the planet where no tile
// is. Never blocks and never starts a build, so the same request gets sharper as tiles arrive.
// Water is drawn to be read at any pitch. `waters` and `water` are readSurface's at the pixel
// centre, with two exceptions where pixels are narrower than cells: a stream is the path from the
// centre of its cell to the centre of the cell it flows into, not the whole cell, and water
// standing next door reaches in where the pixel's own ground lies below it. `discharge` is the
// largest stream a pixel covers or lies on.
// It only reads the planet and finished tiles, neither of which changes once it exists: any number
// of threads may read charts while tiles are being built and while the session runs. For the same
// reason a chart is the untouched world: what the session has changed (overlayLive) belongs to the
// thread that drives it and is not read here.
Chart chartOf(const World& world, const ChartRequest& request);

} // namespace derivean::sim
