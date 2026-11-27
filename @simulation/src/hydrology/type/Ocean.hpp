#pragma once

#include "hydrology/type/Basins.hpp"
#include "hydrology/type/WaterGrid.hpp"

#include <memory>

namespace derivean::sim {

// A channel cut between two cells of the planet grid, its bed level along its whole length. It has
// no valve: water runs through it down whichever way the surfaces at its ends fall, as it does
// through Gibraltar or the Bosporus. Which end is `a` only fixes the sign of what it moved.
struct Strait {
    int32_t a{}, b{};   // planet cells at its ends
    double area{};      // m² of its throat
    double sill{};      // m, its bed; it acts at no less than the ground at either end
};

// One connected stretch of open water on the planet grid.
struct SeaBody {
    double level{};    // m
    Tally stock;
    int cells{};
    int straits{};     // throats with an end in it
    bool ocean{};      // it holds Ocean::anchor
};

// The surface water of the planet as a finite stock, held in the bodies the planet's depressions
// make. The solid surface is fixed for a session, so its depressions are found once; what changes
// is how full they are. Straits move water between bodies and nowhere else: what the planet holds
// stays what it held at the start, the floor letting nothing through. A sea that loses its
// connection across a sill keeps its own level, water and salt from then on. The salt of a dry
// cell is its crust, which dissolves when water covers it again.
struct Ocean {
    WaterGrid grid;        // the planet grid: every cell open, both axes wrapping
    BasinTree basins;
    Lakes lakes;
    std::vector<Strait> straits;
    std::vector<Tally> moved;   // by strait: what went through it, positive from `a` to `b`; it
                                // describes the throat, not the bodies, whose parts it can mix up
    int32_t anchor{-1};         // deepest cell of the largest body at the start: the cell the ocean is known by
    double pristineLevel{};     // m, the sea level every landscape tile was built for
    double salinity{};          // kg/m³ at the start
    Tally initial;              // what the planet held at the start, its crust's salt with it
    double initialCrust{};      // kg, the salt of the dry cells at the start
    Tally unbooked;             // largest share of one body's water and salt in one integration
                                // that its account (segments, crust, parting, meeting) does not
                                // explain; a diagnostic
    double seconds{};           // time the sea has been integrated for
    double pending{};           // s gathered since; see drainOcean
    double opened{};            // `seconds` at which the newest strait was cut
    uint64_t changes{};         // integrations that changed a body's level or which cells are wet
    std::shared_ptr<const Ocean> start;   // the ocean the session began with where it was not filled
                                          // from the planet, for a jump back to the start
};

} // namespace derivean::sim
