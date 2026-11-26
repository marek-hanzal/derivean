#pragma once

#include "icesheet/type/Sheet.hpp"
#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// m³ of rock and cover one working of the bed moved. worn + entered = lodged + dropped + left.
struct WearTotals {
    double worn{};      // taken off the domain's free cells
    double entered{};   // brought in by ice from beyond them
    double lodged{};    // laid under the ice as till
    double dropped{};   // laid where the ice ended
    double left{};      // carried out of the free cells
};

// Cells of a domain from the highest ice surface down; equal heights in index order.
std::vector<int32_t> downIceOrder(const SheetField& field);

// Wear.cpp — a stage's ice slides over the ground for `years`: it strips the loose cover, grinds
// the rock in proportion to the distance it slides and to how readily that rock breaks, and stops
// where the bed would rise too steeply along the flow or its front could no longer stand in the
// sea. Fills `debris` (m³ per cell and class) with what each cell lost. `span` is the stretch of
// the replay the working stands for; with one, the tile's erosion rates take it up, so the plate
// floats back as it does for what rivers remove.
void wearBed(Domain& domain, SheetWork& work, const SheetField& field, const std::vector<int32_t>& downIce, double years, double span,
             std::vector<float> debris[SedimentClasses], WearTotals& totals);

// Debris.cpp — the ice carries that debris down its surface: a share is lodged under it on the
// way, what reaches ground the ice does not cover stays there. `cycles` is the number of
// glaciations the working stands for. `tunnel` (m³/yr per cell, or null)
// marks the streams under the ice: where one is large enough it takes gravel and sand out of the
// ice and keeps it in its tunnel. With `left`, the deposits are also booked by kind.
void carryDebris(Domain& domain, const SheetField& field, const std::vector<int32_t>& downIce, std::vector<float> debris[SedimentClasses], double years, double cycles,
                 double span, const std::vector<float>* tunnel, SheetRecord* left, WearTotals& totals);

// How readily the rock at the top of a cell breaks under ice, against sound granite.
float rockFactor(const Domain& domain, size_t i);

} // namespace derivean::sim
