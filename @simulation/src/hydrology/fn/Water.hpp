#pragma once

#include "hydrology/constant/Water.hpp"
#include "hydrology/type/Basins.hpp"
#include "hydrology/type/WaterGrid.hpp"

namespace derivean::sim {

// Sizes every per-cell array of a grid of Open cells with no water.
void shapeGrid(WaterGrid& grid, int width, int height, double cell);

// Basins.cpp — the nested depressions of the grid's solid surface.
BasinTree buildBasins(const WaterGrid& grid);

// Settle.cpp — hydrostatics. Level of a basin holding `own` m³ above its base, and the reverse.
double levelOf(const BasinTree& tree, int32_t basin, double own);
double volumeOf(const BasinTree& tree, int32_t basin, double level);
void refreshLakes(const BasinTree& tree, Lakes& lakes);

// Lets all water of the grid, plus `parcels` (indexed by basin; water may only enter at leaves,
// which is where BasinTree::leafOf sends it), run downhill until it stands level: pits fill, full
// basins overflow into the next, basins that meet become one lake. Salt travels with the water and
// every lake ends mixed.
Lakes settle(WaterGrid& grid, const BasinTree& tree, std::vector<Tally> parcels, WaterFlows& flows);

// The way `settle` pours, for water entering at one leaf: a full basin passes it up into its
// parent once its sibling is full too, otherwise across its saddle. `pourTarget` is the first
// basin with room, 0 when the water would leave the grid; `pour` adds `volume` to `own` and
// returns what left the grid.
int32_t pourTarget(const BasinTree& tree, const std::vector<double>& own, int32_t leaf);
double pour(const BasinTree& tree, std::vector<double>& own, int32_t leaf, double volume);

// Steady.cpp — the quiet solver: what `years` of river inflow and `climateYears` of rain, runoff
// and evaporation leave behind once the water has come to rest.
Lakes fillAndSpill(WaterGrid& grid, const BasinTree& tree, double years, double climateYears, WaterFlows& flows);

// Writes WaterGrid::discharge for the present state: lakes that are full pass their water on,
// lakes still filling keep it.
void routeDischarge(WaterGrid& grid, const BasinTree& tree, const Lakes& lakes);

// Sediment.cpp — `years` of sediment on the present discharge: entering load and what beds cut by
// more stream power than they used to carry lose settle where the current dies. True when ground
// changed.
bool moveSediment(WaterGrid& grid, const BasinTree& tree, const Lakes& lakes, double years, SolidFlows& flows);

// What drives a bed down, per unit of erodibility: (Q^m − Q0^m)·slope towards the cell it drains
// to, for the grid's present discharge and the tree's surface. Zero for pits and for flows too
// small to cut. Run on the untouched surface it gives WaterGrid::balance.
double streamPower(const WaterGrid& grid, const BasinTree& tree, size_t cell);

// Top of a column as every reader stacks it: rock, then coarse, sand and fine cover.
inline float groundOf(const Ground& solid, size_t cell) {
    return solid.rock[cell] + solid.sediment[Coarse][cell] + solid.sediment[Sand][cell] + solid.sediment[Fines][cell];
}

// Take up to `wanted` metres off the top of a column (cover as it lies, fines first, then rock) or
// lay them on it. Both return the metres the stored floats actually changed by, which is what a
// ledger must book so that it cannot drift from the columns by rounding.
double cutColumn(Ground& solid, size_t cell, double wanted);
double layColumn(Ground& solid, size_t cell, double wanted, SedimentClass layer);

// Surge.cpp — the event solver: `seconds` of shallow water with inertia and bed friction on a
// stable step of its own. Returns the fastest change of depth it saw, m/s.
double surge(WaterGrid& grid, double seconds, WaterFlows& flows, SurgeScratch& scratch);

} // namespace derivean::sim
