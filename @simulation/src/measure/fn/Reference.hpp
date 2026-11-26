#pragma once

#include "measure/type/Mosaic.hpp"

namespace derivean::sim {

// Hand-made surfaces with a known answer. The regularity indicators are read on them to show what
// "no pattern" and "pattern" measure as, which is where their thresholds come from.

// Seeded fractal relief: every wavelength present, none preferred.
Field<float> fractalRelief(uint64_t seed, int side, double wavelength, double height, double gain = 0.5);

// A plane falling along +X with straight gullies `spacing` cells apart, `depth` deep, under noise.
Field<float> combRelief(uint64_t seed, int side, double spacing, double depth, double noise);

// Ridges of one height `spacing` cells apart crossing the surface at an angle, under noise.
Field<float> sawRelief(uint64_t seed, int side, double spacing, double height, double noise);

// A cone draining to its middle, with water on the disc inscribed in the square, so every heading
// has the same number of cells. Locked, every cell flows to its steepest neighbour and paths run
// along the eight grid directions; otherwise the choice between the two steepest is drawn so that
// a path follows the true fall line on average.
Mosaic coneMosaic(uint64_t seed, int side, bool locked);

} // namespace derivean::sim
