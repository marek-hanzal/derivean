#pragma once

#include "landscape/fn/Build.hpp"

namespace derivean::sim {

// Seed.cpp — the wind's side of a domain at the start of its window: the regime, the labels of the
// parent and of finished neighbours, and at Local and Site the wind-laid sand parked off the ground.
// Called once the neighbours are adopted and before the surface a finer level is held to is read.
void beginWind(Domain& domain, const Setting& setting, const TileLookup& lookup);
// Sizes the wind's arrays of a hand-built domain; its regime is the caller's to set.
void allocateWind(Domain& domain);
// The labels over the tile's own cells, none above its class.
WindRecord windRecordOf(const Domain& domain);

// Cover.cpp — how the cover meets the wind.
// True where the step's flood stands above the ground. Reads `level` against the `surface` the flood
// was found on, which a caller must therefore leave as the flood left it, or against the ground
// with its parked sand where that stands higher.
bool underFlood(const Domain& domain, size_t i);
// True where water or ice holds the ground: the sea, a lake its rivers still reach, a stream at
// least half a cell wide, a glacier. Reads the flood of the step through `underFlood`.
bool heldByWater(const Domain& domain, size_t i);
// m of sand the wind could take from a dry cell before the stones it leaves close the surface.
float mobileSand(const Domain& domain, size_t i);
// Takes `amount` metres of sand out of a cell's cover: wind-laid sand first, then sand out of the
// mixed cover, whose stones stay as lag and whose fines leave as dust. Returns the dust in metres.
float takeSand(Domain& domain, size_t i, float amount);
void layBlown(Domain& domain, size_t i, float amount);
// Enters metres the wind's side laid on a cell (positive) or took from it (negative) in the tile's
// own record of deposition and erosion, so the tile's ledger still says where its ground went.
void bookWind(Domain& domain, size_t i, float metres);
void tidyLabels(Domain& domain);

// DuneField.cpp — Local and Site: lays the parked sand back and lets the transport model work the
// tile for the span of its level.
void shapeDunes(Domain& domain);

// True during the last step of a domain's replay (stepDomain advances `time` after the hook runs).
bool lastStep(const Domain& domain);

// Budget.cpp — Region: one step of the sand budget. Sand the wind can take moves along the resultant
// drift, limited by what the wind can carry and by what the ground gives, into water and into the
// shelter of relief. `raised` receives the metres of fines each cell sent up as dust on the way.
// Cells from upwind to downwind along the resultant drift, and for each cell the two neighbours it
// hands on to with their shares.
struct WindGraph {
    std::vector<int32_t> order;
    std::vector<int32_t> to;        // cells × 2, -1 beyond the domain
    std::vector<float> share;       // cells × 2
};
WindGraph windGraph(const Domain& domain);
void routeSand(Domain& domain, const WindGraph& graph, std::vector<float>& raised);

// FarDust.cpp — Region: `WindField::farDust`, a closed form over the planet grid of the dust that
// upwind deserts raise and the annual mean wind brings to each cell, a function of world position. Called by `beginWind`.
void readFarDust(Domain& domain, const Setting& setting);

// Dust.cpp — Region: fines raised this step travel with the same drift and settle with distance,
// and the far dust settles on every cell of the tile's own.
void settleDust(Domain& domain, const WindGraph& graph, const std::vector<float>& raised);

// Soak.cpp — m³/yr of the `discharge` passing a cell that its loose bed takes in floods instead of
// passing on, at most `discharge`: transmission losses of dry country, which the groundwater takes
// as recharge. None under standing water or ice, on cells that are not Free, or where rain keeps up
// with the air. Called with the water that has reached the cell, its own yield included.
float sinkIntoBed(const Domain& domain, size_t i, float discharge);

// Basin.cpp — Region: what a basin's water leaves where it leaves only as vapour.
void dryBasins(Domain& domain, FlowScratch& scratch);

} // namespace derivean::sim
