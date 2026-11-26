#pragma once

#include "icesheet/type/Continental.hpp"
#include "landscape/type/Terrain.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

struct Planet;

// What a square of finished tiles of one level holds of the ice and of what it left. Shares are of
// the cells of the square unless said otherwise.
struct SheetCensus {
    size_t cells{};
    double iceNow{}, iceThen{};              // under ice now, and at the last maximum
    double thickestNow{}, thickestThen{};    // m
    double freed{};                          // ground the last sheet covered that is bare of ice now
    double freedLakes{};                     // share of that ground under standing water above the sea
    double freedSea{};                       // share of it under the sea
    double deepestBed{};                     // m against the sea level: lowest ground under the last sheet
    double wornMean{}, wornMost{};           // m of rock ground off, over the ground the sheets wore
    double till{}, moraine{}, outwash{}, esker{};                    // share of the cells holding half a metre or more of each
    double tillDepth{}, moraineDepth{}, outwashDepth{}, eskerDepth{};   // m, mean where it lies
    double slide{}, fastest{};               // m/yr: mean and greatest sliding of the last maximum's ice
};

SheetCensus censusOf(const std::vector<const Tile*>& tiles);

// A place the search names for a look: world coordinates (as tiles and tools take them).
struct IcePlace {
    Vec2 at;
    double height{};      // m above the sea, of the planet cell
    double thickness{};   // m of ice over it at the last maximum
};

// Planet cells the present ice does not cover and the last sheet did: where fresh glacial country
// is to be looked for (`limit` of them, no two close together). Inland: ice worth the name, no sea
// near, the lowest ground first. Coastal: open sea beside, the highest first. Margin: inland, thin
// ice (where it stagnated as it withdrew) with ground the sheet never reached within two cells,
// the lowest first.
enum class PlaceKind : uint8_t { Inland, Coastal, Margin };
std::vector<IcePlace> freedPlaces(const Planet& planet, const ContinentalIce& ice, PlaceKind kind, int limit);

} // namespace derivean::sim
