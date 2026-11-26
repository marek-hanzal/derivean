#pragma once

#include "landscape/type/Terrain.hpp"
#include "measure/type/Mosaic.hpp"
#include "measure/type/ReliefStats.hpp"

namespace derivean::sim {

// How rugged a piece of ground is, in the terms a DEM of the same cell size is described in. Every
// function reads one mosaic; none builds tiles.

// Slopes.cpp
// Rise over run of every cell by Horn's gradient; negative where the cell or a neighbour is not dry land.
Field<float> slopeField(const Mosaic& mosaic);
// Steepest fall from a cell to one of its eight neighbours, rise over run: what a face is judged by.
Field<float> steepestFall(const Mosaic& mosaic);
SlopeStats slopeStats(const Field<float>& slope);
WindowRelief windowRelief(const Mosaic& mosaic, double window);
CoverShares coverShares(const Mosaic& mosaic, const Field<float>& slope, const Field<float>& fall);

// Valleys.cpp
DrainageStats drainageDensity(const Mosaic& mosaic);
ValleyStats valleySections(const Mosaic& mosaic);

// Crests.cpp
CrestStats crestSharpness(const Mosaic& mosaic);

// Cliffs.cpp
CliffStats cliffInventory(const Mosaic& mosaic, const Field<float>& fall);

// Slopes.cpp — everything above for one mosaic.
ReliefReport reliefReport(const Mosaic& mosaic);

// Slopes.cpp — the cells a domain owns as a mosaic of one tile, so a hand-built block is measured
// by the same functions as the world. Flow directions are the steepest fall of the flooded surface.
Mosaic mosaicOf(const Domain& domain);

} // namespace derivean::sim
