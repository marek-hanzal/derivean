#pragma once

#include "landscape/type/Terrain.hpp"

#include <functional>

namespace derivean::sim {

// Working arrays of the steps of one replay, allocated once. `retreated`, `left`, `parentLift` and
// the soundness arrays carry over from step to step, so a scratch belongs to one domain for the
// whole replay.
struct FlowScratch {
    std::vector<float> share;                    // cells × 8: part of a cell's outflow that leaves towards each neighbour
    std::vector<float> next;                     // m, surface after cutting and filling
    std::vector<float> flux[SedimentClasses];    // m³/yr of sediment passing through
    std::vector<float> settle[SedimentClasses];  // m laid down this step, by class
    std::vector<float> plume[SedimentClasses];   // m³ of sediment suspended in standing water
    std::vector<float> power, coverPower;        // K·dt·(Q^m - threshold)/cell, the implicit erosion factor of a cell's rock and of the loose cover on it
    std::vector<float> room;                     // m a bed can rise before it dams a cell upstream
    std::vector<int32_t> head, link, pit;        // bucket queue of the flood
    std::vector<uint8_t> visited;
    std::vector<float> moved[SedimentClasses + 1];   // slope transfers; the last entry is rock
    std::vector<float> broke;                    // m of rock each cell lost to failure this step
    std::vector<float> blurA, blurB;
    std::vector<float> work;                     // m, surface the slope processes of a step work on
    std::vector<int32_t> byHeight, spare;        // cells from the lowest up, by the surface the pass that sorted them worked on
    std::vector<int32_t> foot;                   // cell at the foot of the slope each cell stands on
    std::vector<float> retreated;                // m the free face of each column has weathered back since the column last came down
    double left{};                               // m³ of debris that slid onto cells that do not evolve, over the whole replay and domain
    std::vector<float> soundScale, soundLog;     // how sound each cell's rock is against its material: fixed for a replay
    std::vector<uint8_t> drained;                // 1 where a lake of this step lost its water to the cutting of its outlet
    std::vector<float> pool;                     // m, water surface each cell is left with once the beds below it are cut
    std::vector<float> travelled;                // routing only: the way a cell's inflow has come, summed over that inflow
    std::vector<float> gathered;                 // m³/yr of a cell's water beyond the sheet a plane of the same length would bring it
    std::vector<float> delivered[SedimentClasses];   // m of broken rock a cell hands to its water this step; empty where debris stays as talus
    std::vector<float> parentLift;               // m/yr, a finer level's rock uplift as its parent handed it over; set at the first step of a led replay, empty otherwise
};

// Rows.cpp — runs `body(begin, end)` over the rows [0, count) of a domain: spread over its pool
// where it has one, on the calling thread otherwise. `body` may write only what belongs to its own
// rows and must not sum across them, so the result is the same however the rows are cut.
void forRows(const Domain& domain, int count, const std::function<void(int, int)>& body);

// Flood.cpp — fills every depression with standing water and orders cells from the outlets upstream.
void floodLevels(Domain& domain, FlowScratch& scratch);

// Flow.cpp — spreads each cell's water over its lower neighbours and accumulates discharge.
void routeWater(Domain& domain, FlowScratch& scratch);

// Fluvial.cpp — stream-power incision with transport-length deposition, solved implicitly.
void cutAndFill(Domain& domain, FlowScratch& scratch);

// Plume.cpp — sediment that reached standing water this step spreads from where it entered and
// settles; `dt` and `area` are the step in years and the cell in m².
void settlePlume(Domain& domain, FlowScratch& scratch, float dt, float area);

// Hillslope.cpp — regolith production, solution, creep and slope failure.
void shapeSlopes(Domain& domain, FlowScratch& scratch);

// Stability.cpp — free faces retreat as they weather, and rock that stands higher than its strength
// allows over any run breaks; both leave their debris on the cell. Region, Local and Site levels.
void breakSlopes(Domain& domain, FlowScratch& scratch);
// Share of the year a cell's rock spends in the frost-cracking window.
float frostShare(const Domain& domain, size_t i);
// Cells in rising order of `height`, ties in index order.
void orderByHeight(const std::vector<float>& height, std::vector<int32_t>& order, std::vector<int32_t>& spare);

// Talus.cpp — loose cover comes to rest at its angle of repose; coarse debris breaks down.
void settleDebris(Domain& domain, FlowScratch& scratch);
void wearDebris(Domain& domain);

// Glacial.cpp — ice from the snow balance of a sinusoidal year, its erosion and its moraines.
void glaciate(Domain& domain, FlowScratch& scratch, float seasonalHalfRange, float cooling, bool erode);

// aeolian/fn/Budget.cpp — wind moves dry loose sand and dust and lays them down; dry basins keep their salt; at Local and Site the dune field (aeolian/fn/Wind.hpp).
void blowLoose(Domain& domain, FlowScratch& scratch);

// coast/fn/Shore.cpp — waves on seas and lakes cut rock back at the waterline and sort and carry sand and gravel (coast/fn/Coast.hpp).
void workShores(Domain& domain, FlowScratch& scratch);

// volcanic/fn/Erupt.cpp — the vents of the domain lay the lava, fall and ash of this step and found what they emptied (volcanic/fn/Volcanic.hpp).
void erupt(Domain& domain, FlowScratch& scratch, const struct Setting& setting);

// icesheet/fn/Sheet.cpp — continental ice (icesheet/fn/Sheet.hpp says what each stage does). Step: the sheets of the glaciations a region step
// holds wear its bed and lay their debris. Leave: after the last step, what the last sheet stripped, cut and left. Present: the ice standing
// now; returns the tile's record. `tiles` answers for the finished tiles a builder may ask for (the parents of a finer level).
enum class IceStage : uint8_t { Step, Leave, Present };
std::shared_ptr<const SheetRecord> sheetIce(Domain& domain, FlowScratch& scratch, const struct Setting& setting, IceStage stage,
                                            const std::function<const Tile*(TileKey)>* tiles = nullptr);

// Units.cpp — keeps the cached rock traits in step with how deep the surface has cut.
void refreshUnits(Domain& domain, uint64_t seed);

// Coefficients shared by the processes.
inline constexpr double StreamExponent = 0.45;        // discharge exponent of stream power
inline constexpr float ChannelHead = 3000.0f;          // m³/yr below which running water does not incise
// A cell of the coarser levels is far wider than the rills on it and hides them; a ground cell is
// a rill's width, so there the water of a few tens of square metres already cuts.
inline constexpr float GroundChannelHead = 60.0f;
// A level cuts only the valleys it can draw. Below the water of a few tens of its cells the ground
// is slope, not channel: a Region cell of a square kilometre holds its headwater streams inside
// itself, a Local cell its gullies, a Site cell its rills. Rock there is lowered at the pace of the
// largest stream it hides; the channels it hides belong to the next level. Region: about 28 km²
// under a metre of runoff; Local: about 0.1 km², some 25 of its cells; Site: 400 m², 25 cells.
inline constexpr float ChannelHeads[LevelCount] = {0, 2.5e7f, 1.0e5f, 400.0f, GroundChannelHead};
inline constexpr float channelHeadOf(Level level) { return ChannelHeads[size_t(level)]; }
// The Region, Local and Site levels draw their valleys by the water a cell has gathered beyond the
// sheet any slope carries, lower the ground between them at the pace of the streams a cell hides,
// and let a stream keep to one drawn path. Ground cuts with all the water of a cell.
inline constexpr bool gathersWater(Level level) { return level != Level::Ground; }
// m³/yr of the largest stream a cell holds inside itself where a metre of water runs off in a year:
// a stream just short of a channel of the level. A cell that has gathered less water than that is
// lowered at the pace such a stream cuts on the cell's slope. A Site cell is a few metres wide and
// hides rills at most: its slopes are lowered by the weather, creep and failure alone.
inline constexpr float HiddenStreams[LevelCount] = {0, 2.5e7f, 1.0e5f, 0, 0};
// A ground cell also meets a slope's water as it comes: in storms, at many times the yearly mean
// the discharge stands for. The less water a cell gathers the flashier it is; a stream of the size
// the coarser levels cut with carries its mean most of the time, and the gain is gone.
inline constexpr float GroundStormGain = 9.0f;
// Its parent has already struck the balance between the river and the rock of its bed. A ground
// tile's water works the cover; rock it cuts only by this share of what the same law gives a wider
// cell, enough to step the bed over a strong layer and not enough to saw a slot along a weak one.
inline constexpr float GroundRockCut = 0.08f;
inline constexpr float rockCutOf(Level level) { return level == Level::Ground ? GroundRockCut : 1.0f; }
inline constexpr float stormGainOf(Level level, float discharge) {
    return level == Level::Ground ? 1.0f + GroundStormGain / (1.0f + discharge / ChannelHead) : 1.0f;
}
inline constexpr float SedimentErodibility[SedimentClasses] = {2.2e-5f, 8.0e-5f, 5.5e-5f};
inline constexpr float SettlingNumber[SedimentClasses] = {2.4f, 0.9f, 0.16f};   // dimensionless transport length
inline constexpr float BedloadWear = 2.0e-5f;          // share of coarse load ground down per metre travelled
inline constexpr float ReposeTan[SedimentClasses] = {0.78f, 0.62f, 0.70f};
inline constexpr float DebrisFrost = 10.0f;            // gain of the breakdown of rock and debris per unit share of the year spent cracking

} // namespace derivean::sim
