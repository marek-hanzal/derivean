#pragma once

#include "measure/type/Mosaic.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {

// The planet in the quantities of the "Earth as a whole" table of docs/reference-landscapes.md.
// Shares are of land area unless noted.
struct PlanetLikeness {
    double landShare{};         // of the whole surface
    double bands[7]{};          // land below 200 m, to 500, 1 000, 2 000, 3 000, 4 000 m, and above
    double meanHeight{};        // m above the sea
    double quartiles[3]{};      // m
    double sedimentary{}, carbonate{}, metamorphic{}, plutonic{}, volcanic{};   // rock at the top of the layering
    double dryland{};           // rain under 65 % of what the air can take back (the demand runoffOf uses)
    double inland{};            // land whose steepest path over the planet grid ends in a hollow above the sea
    double straightShore{};     // of shore cells (measure/fn/Regularity.hpp)
};

PlanetLikeness likenessOf(const Planet& planet);

// The same table's rows that need kilometre cells, gathered over Region mosaics.
struct ReliefLikeness {
    size_t land{};              // dry cells read
    size_t windows{};           // 55 km windows with land in them
    double reliefOver200{};     // share of land in 5 km windows whose relief exceeds 200 m
    double lakes{};             // share of the land's cells under standing fresh water
    // Meybeck's classes from 55 km windows: relief roughness is the height range of the window
    // over half its width, in m/km.
    double plains{}, lowlands{}, hills{}, plateaus{}, mountains{};
    double roughness[7]{};      // under 5, to 10, 20, 40, 80, 160 m/km, and above
};

// Adds one mosaic to running sums; `finish` turns the sums into shares.
void addRelief(const Mosaic& mosaic, ReliefLikeness& sums);
void finish(ReliefLikeness& sums);

} // namespace derivean::sim
