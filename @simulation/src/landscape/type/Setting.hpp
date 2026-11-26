#pragma once

#include "landscape/type/Drainage.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {

// Planet-scale inputs every level reads, derived once from the evolved planet. All fields are on
// the planet grid and describe the region window: from `regionStart` to the planet's present.
struct Backdrop {
    Field<float> pastElevation;   // m at regionStart, of the crust that is at each cell now
    Field<float> thickening;      // m/yr of crust added by tectonics and magmatism
    Field<float> crust;           // m of felsic crust now
    Field<float> residual;        // m/yr of surface motion that is not crustal: lid cooling, mantle swell
    Field<float> discharge;       // m³/yr of the planet-scale rivers
    Field<float> load;            // m³/yr of sediment they carry
    Field<float> graded;          // m, bed a river in balance with present erosion would have on its way to the sea
    Field<uint8_t> flowTo;        // neighbour index they leave by, 255 for none
    Drainage drainage;            // the same rivers on a grid fine enough to say where they cross a region's edge
    double regionStart{};
};

class Workers;

// `workers` spreads the work over a pool; null runs it on the calling thread. The result is the same.
Backdrop readBackdrop(const Planet& planet, double age, Workers* workers = nullptr);

struct World;

// Identity of a world. Everything a tile contains is a function of this and of its key.
struct Setting {
    uint64_t seed{};
    double age{};
    const Planet* planet{};
    const Backdrop* backdrop{};
    // The world the setting belongs to, for the one thing a tile asks of another model: the springs
    // of the groundwater under a ground tile. Null in a bare setting, which then has none.
    World* world{};
    // Not part of the identity: the pool a tile's own work may be spread over, set by the atlas that
    // builds with it. What a tile contains does not depend on it; without one a tile is built on the
    // calling thread alone.
    Workers* workers{};
};

} // namespace derivean::sim
