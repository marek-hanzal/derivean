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
    double freed{};       // share of the square that is ground of the kind asked for (region squares only)
};

// Planet cells the present ice does not cover and the last sheet did: where fresh glacial country
// is to be looked for (`limit` of them, no two close together). Inland: ice worth the name, no sea
// near, the lowest ground first. Coastal: open sea beside, the highest first. Margin: inland, thin
// ice (where it stagnated as it withdrew) with ground the sheet never reached within two cells,
// the lowest first.
// Shield: inland on crystalline rock; on the planet map, which knows no rock at the surface, as inland.
enum class PlaceKind : uint8_t { Inland, Coastal, Margin, Shield };
std::vector<IcePlace> freedPlaces(const Planet& planet, const ContinentalIce& ice, PlaceKind kind, int limit);

// The squares of one finer tile inside a finished region tile whose own record holds most ground of
// the kind, judged on its own 1 km cells: covered at the last maximum, bare now, above the sea
// (ground at or below sea level counts as sea), with sea or never-iced ground looked for within
// two cells inside the tile only. Unlike the planet search, any ice at the maximum counts, a
// coastal cell needs only sea beside it, and Margin means ice under PressingIce, where hollows
// stay open as kettles. The planet map is too coarse for this: on seed 42 the region's own
// present ice covers 90 % of the tile around a planet margin place. At the centre of each square;
// `height` and `thickness` are means over the cells counted.
std::vector<IcePlace> freedSquares(const Tile& region, PlaceKind kind, int limit);

} // namespace derivean::sim
