#pragma once

#include "planet/type/Planet.hpp"

namespace derivean::sim {

// The whole planet in a few numbers: what must stay bounded however long it is evolved.
struct PlanetSummary {
    double time{};            // yr
    double seaLevel{};        // m
    double landShare{};       // share of the surface above the sea
    double meanLand{};        // m above the sea
    double highest{};         // m above the sea
    double high99{};          // m above the sea that one land cell in a hundred exceeds
    double above2km{};        // share of the land standing more than 2 km above the sea
    double deepest{};         // m below the sea, negative
    double thickest{};        // m of felsic crust
    double meanCrust{};       // m
    double crustVolume{};     // m³ of felsic crust including loads in transit
    double coverVolume{};     // m³ of dated sedimentary cover
    double straightShore{};   // share of shore cells in runs of five or more along a grid axis (measure/fn/Regularity.hpp)
    PlanetLedger ledger;
};

PlanetSummary summarize(const Planet& planet);

} // namespace derivean::sim
