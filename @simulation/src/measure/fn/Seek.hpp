#pragma once

#include "simulation/type/World.hpp"

#include <string>

namespace derivean::sim {

// Kinds of country a world should hold, found from what the planet and the tiles say.
enum class Country : uint8_t {
    HumidRange,     // the highest ground under heavy rain that stays above freezing
    ColdRange,      // the highest ground that stays below freezing
    OldRange,       // a collision belt long since worn down
    DryPlateau,     // high, level, dry ground crossed by a large river
    FlatBeds,       // thick sandstone or limestone that was never folded, standing high enough to be dissected
    FoldBelt,       // folded cover of a collision belt
    Granite,        // a pluton at the surface in high ground
    Lowland,        // gentle, rained-on country far from any range
    Coast,          // the shore the game lands on
    Count,
};

inline constexpr const char* CountryNames[size_t(Country::Count)] = {"humid-range", "cold-range", "old-range", "dry-plateau",
                                                                    "flat-beds",   "fold-belt",  "granite",   "lowland", "coast"};

struct Sought {
    bool found{};
    Vec2 planet;       // best sample of the planet-wide search
    Vec2 local;        // centre of the Local tile chosen from the Region tiles around it
    std::string why;   // what the planet says there
};

// Looks over the whole planet on a lattice of 32 km, then over the Region tiles around the best
// sample for the 16 km of it that show the kind most strongly. Builds those Region tiles.
Sought seekCountry(World& world, Country kind);

// Centre of the Site tile with the most relief inside the Local tile at `local`. Builds that Local tile.
Vec2 steepestSite(World& world, Vec2 local);

} // namespace derivean::sim
