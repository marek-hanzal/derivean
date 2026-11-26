#pragma once

#include "icesheet/type/Sheet.hpp"
#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// Region.cpp — one stage's sheet over the ground a domain has now.

// True if any stage brings ice into the domain: from beyond its rim, or because its own high
// ground holds snow in the coldest stage.
bool iceReaches(const Domain& domain, const SheetSource& source);

// Relaxes a stage on cells four domain cells wide, from the thickness it was left with (or, the
// first time, from what the source holds there). Along the rim the sheet is the source's wherever
// the source has ice; elsewhere the rim is a wall, so an icefield of the domain's own neither
// drains out nor is fed. `towards` (0 to below 1) moves the stage that far to the next colder one:
// its climate and the source's sheet are blended between the two, which lets the margins of the
// same stage stand in different places from one working of the bed to the next. With `recorded`
// 0 or 1 (IceNow, IceLastMaximum), cells a finished neighbour owns are held at the ice that
// neighbour recorded for that stage: the later tile continues the earlier one's sheet, which is
// what makes two tiles agree on their shared edge.
void solveStage(const Domain& domain, SheetWork& work, int stage, double towards, int recorded, Workers* workers);

// Mean of the square of half-width `radius` around every cell of an n × n grid, clamped at the edge.
std::vector<float> boxMean(const std::vector<float>& field, int n, int radius);

// The stage on the domain's own cells: its surface is the relaxed one, its thickness that surface
// over each cell's own ground, so a valley under the sheet carries more ice than the ridge beside it.
SheetField readStage(const Domain& domain, const SheetWork& work, int stage, Workers* workers);

} // namespace derivean::sim
