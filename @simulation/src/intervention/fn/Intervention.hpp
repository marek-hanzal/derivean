#pragma once

#include "simulation/fn/Query.hpp"

namespace derivean::sim {

// One thread drives time and makes edits. Readers of the overlay (queries, meshing) must not run
// while `intervene`, `advance`, `tick`, `jumpTo` or `syncLive` are running.

// Clock.cpp — the outside authority's handles on time. A date is the session's era plus calendar
// seconds; the era belongs to the world and does not move while the world exists.
void intervene(World& world, Edit edit);        // stamps the edit with the present, records it and applies it
void advance(World& world, double seconds);     // calendar seconds, whatever the rate
void tick(World& world, double realSeconds);    // advances by rate × realSeconds unless paused
// To calendar second `seconds` of the session. Forwards is `advance`; backwards rebuilds the state
// from the untouched world and the history up to that second, which is also how a saved history
// is loaded. Edits made later stay in the history and take effect when their time comes again.
void jumpTo(World& world, double seconds);
// Brings every live group and the sea to the present. Groups in which nothing is happening are
// otherwise advanced only once an hour of calendar time has gathered, and the sea once a day has.
// Where a group's steps end shows in the last bits of its state, so a sync is part of the history
// of calls: two runs agree to the bit only if they sync at the same moments.
void syncLive(World& world);

// Active.cpp — live tiles and the groups they form.
EditedTile& activate(World& world, TileKey key);
void regroup(World& world);                     // rebuilds the grids of groups whose tiles changed
void storeSet(LiveSet& set);                    // refreshes the tiles from the group's grid
std::vector<TileKey> tilesWithin(Vec2 low, Vec2 high);

// Raster.cpp — puts one stroke or deposit into the live tiles' columns and books what it removed and added.
void rasterise(World& world, const Edit& edit);
void strokeBounds(const Edit& edit, Vec2& low, Vec2& high);   // the box around a stroke's footprint

// Evolve.cpp — moves one group's water from where it stands to calendar second `to`.
void evolve(World& world, LiveSet& set, double to, bool force);
void shapeSet(World& world, LiveSet& set);      // sediment up to the group's present

// Surface.cpp — what the rest of the simulation and the game read.
// The live tile under a key, or null where the world is untouched.
const EditedTile* editedTile(const World& world, TileKey key);
// Replaces what `sample` says of ground, cover, water and discharge by the live state where there
// is one. `sample` must come from the site level at `position`.
void overlaySurface(const World& world, Vec2 position, SurfaceSample& sample);

// One column of the live or untouched site surface, by global site cell. Blocks like surveyAt.
struct SiteColumn {
    float rock{}, cover[SedimentClasses]{}, ground{};   // m
    double depth{};       // m of water standing on it
    double salinity{};    // kg/m³ of that water
    float discharge{};    // m³/yr
    bool live{};
};
SiteColumn siteColumn(World& world, int64_t cellX, int64_t cellZ);

// Budget.cpp — where every cubic metre of water, kilogram of salt and cubic metre of solid is.
struct WorldBudget {
    Tally ocean;          // the body that holds Ocean::anchor
    Tally others;         // every other body
    Tally oceanAtStart;   // what all of them held at the start
    Tally moved;          // what went through the straits, summed without sign
    Tally unbooked;       // worst share of a body's change its straits' transfers did not book
    Tally live;           // standing on live tiles
    Tally adopted;        // what those tiles held when they became live
    WaterFlows flows;     // what crossed their edge since
    double dug{}, filled{};
    SolidFlows solid;
    double groundChange{};   // m³, solid on live tiles now less solid on the same tiles untouched
    double seaLevel{};       // m, surface of `ocean`
};

WorldBudget budgetOf(World& world);   // syncs first
// Share of each budget that is unaccounted for; zero up to rounding.
double waterImbalance(const WorldBudget& budget);
double saltImbalance(const WorldBudget& budget);
double solidImbalance(const WorldBudget& budget);

} // namespace derivean::sim
