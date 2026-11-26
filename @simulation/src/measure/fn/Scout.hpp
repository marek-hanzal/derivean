#pragma once

#include "measure/fn/Catalogue.hpp"
#include "measure/type/Scout.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

// Detectors of showcase sites on one mosaic. Every threshold is in metres, so the same detector
// reads a Region mosaic to find where to look and a Local mosaic to confirm what is there. Each
// returns its best `limit` sites, no two closer than a few kilometres. Camera poses are not set.

// ScoutLand.cpp
std::vector<ScoutSite> findCoasts(const Mosaic& mosaic, int limit);    // shore with land rising under 3 % for 5 km behind it
std::vector<ScoutSite> findPlains(const Mosaic& mosaic, int limit);    // 10 km of land with under 150 m of relief, no water, no cliff
std::vector<ScoutSite> findSaddles(const Mosaic& mosaic, int limit);   // pass with a summit 300 m higher on both sides within 10 km
std::vector<ScoutSite> findColdSummits(const Mosaic& mosaic, int limit);   // granite or gneiss summit that stays below freezing, 600 m over its valleys

// ScoutWater.cpp
std::vector<ScoutSite> findValleys(const Mosaic& mosaic, int limit);   // trunk stream between walls of 150 m
std::vector<ScoutSite> findGorges(const Mosaic& mosaic, int limit);    // cut of 100 m with level ground within 1 km of it
std::vector<ScoutSite> findBasins(const Mosaic& mosaic, int limit);    // lake of 0.05 km² or more above 800 m that spills into a stream

// ScoutRock.cpp
std::vector<ScoutSite> findSandstone(const Mosaic& mosaic, int limit); // steep bare sandstone or conglomerate, layering near level

// ScoutCity.cpp — rock-city ground: sandstone at the surface that is at least 60 m thick within
// 100 m of it, in beds dipping under 3°, on ground with 80 to 300 m of relief within a kilometre.
std::vector<ScoutSite> findRockCities(const Mosaic& mosaic, const Planet& planet, int limit);

// ScoutHeat.cpp — land with rain where the heat from below exceeds 0.15 K/m.
std::vector<ScoutSite> findHotGround(const Mosaic& mosaic, const Planet& planet, int limit);

// Adds one tile of the mosaic to a running count of dry cells and of those findSandstone would accept.
void countSandstone(const Mosaic& mosaic, int tileX, int tileZ, size_t& land, size_t& fit);

// What sampleBlock makes of a square of ground: how much rock has air under it.
struct RockForms {
    int columns{};           // columns with any rock in them
    double roofedShare{};    // share of them with air between rock below and rock above
    double bedThickness{};   // m, thickness of the bed half of the bedded rock lies in or under
    int formations{};        // separate patches of roofed columns
    double spacing{};        // m, mean distance from a patch to its nearest neighbour
    double spacingCv{};      // scatter of that distance: near 0 for evenly spaced copies
    double tallest{};        // m, deepest air gap under a roof
};

RockForms rockForms(World& world, Vec2 centre, double edge = 96.0, double step = 0.5);

// ScoutCamera.cpp — fills both poses of a site from the mosaic it was confirmed on.
void frameSite(const Mosaic& mosaic, ScoutSite& site, double airBack, double airUp);

// ScoutAround.cpp — keeps the best sites that lie at least `apart` metres from every better one.
std::vector<ScoutSite> keepApart(std::vector<ScoutSite> sites, int limit, double apart);

// ScoutAround.cpp — the search itself. It reads the Region tiles within `radius` of the start,
// lets every detector name its best places there, builds Local tiles only around those and keeps
// what the detector still finds at 64 m; Site and Ground tiles are built for the sampled rock blocks alone.
struct ScoutReport {
    Vec2 start;
    std::vector<ScoutSite> coast, saddle, valley, gorge, sandstone, basin, plain, city, summit, hot;
    std::vector<RockForms> forms;   // one per entry of `sandstone`
    std::vector<RockForms> cityForms;   // one per entry of `city`
    std::vector<CatalogueSite> catalogue;   // the best window of every catalogue type met on the Local tiles built
    size_t localLand{};             // dry Local cells looked at
    size_t localSandstone{};        // of them, steep bare sandstone or conglomerate
    size_t regionTiles{}, localTiles{}, siteTiles{};   // built by the search, with everything they waited for
};

ScoutReport scoutAround(World& world, Vec2 start, double radius, int keep = 3);

} // namespace derivean::sim
