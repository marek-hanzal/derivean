#pragma once

#include "hydrology/type/Basins.hpp"
#include "hydrology/type/WaterGrid.hpp"

namespace derivean::sim {

// A passage from the bed of a sea down to the void under the world.
struct VoidOpening {
    int32_t cell{};    // planet cell
    double area{};     // m² of its throat
    double mouth{};    // m, elevation of its upper end; never taken below the sea bed there
};

// One connected stretch of open water on the planet grid.
struct SeaBody {
    double level{};    // m
    Tally stock;
    int cells{};
    bool draining{};   // an opening lies under it
};

// The surface water of the planet as a finite stock. The planet's solid surface is fixed for a
// session, so its depressions are found once; what changes is how full they are. A sea that loses
// its connection across a sill keeps its own level, water and salt from then on.
struct Ocean {
    WaterGrid grid;        // the planet grid: every cell open, both axes wrapping
    BasinTree basins;
    Lakes lakes;
    std::vector<VoidOpening> openings;
    double pristineLevel{};   // m, the sea level every landscape tile was built for
    double salinity{};        // kg/m³ at the start
    Tally initial;            // what the planet held before anything escaped
    Tally escaped;            // gone through openings into the void
    double seconds{};         // time the sea has been integrated for
    double pending{};         // s gathered since; see drainOcean
    double opened{};          // `seconds` at which the newest opening was made
};

} // namespace derivean::sim
