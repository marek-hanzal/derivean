#pragma once

#include "volcanic/constant/Rheology.hpp"
#include "volcanic/type/Emplacement.hpp"

#include <limits>
#include <utility>

namespace derivean::sim {

// Cells whose mobile layer may still have to move, highest surface first, and every cell the layer
// has touched since it was last set.
struct Worklist {
    std::vector<std::pair<float, int32_t>> heap;
    std::vector<int32_t> touched;
};

// Relax.cpp — the one law of motion. A cell passes its mobile layer to lower neighbours while the
// layer's weight along the fall exceeds its yield strength at its present heat; what moves cools by
// the way it travels and sets where its heat is spent. `visits` bounds the cells worked: a replay
// runs to rest, play advances a flow by what one tick allows. Returns the visits used.
void wake(Emplacement& field, Worklist& work, size_t cell);
long relax(Emplacement& field, const Rheology& rheology, Worklist& work, long visits = std::numeric_limits<long>::max());
// Adds `thickness` metres at heat one to a cell's mobile layer.
void feed(Emplacement& field, Worklist& work, size_t cell, float thickness);
// Whatever is still mobile sets where it is: it joins the ground of Open cells.
void set(Emplacement& field, Worklist& work);

// Lava.cpp — one eruption's lava: `volume` m³ fed at a cell a few flow units at a time, each left
// to run out before the next, then set.
void pourLava(Emplacement& field, const Rheology& rheology, int x, int z, double volume);

// What falls out of the air around a vent.
struct Fall {
    Vec2 vent;          // m, world
    double volume{};    // m³ of deposit
    double length{};    // m, e-folding distance of its thickness
    double hole{};      // m, e-folding width of the jet over the vent where nothing falls; 0 for none
    Vec2 stretch;       // downwind, shorter than one: the blanket is an ellipse with the vent at a focus
};

// Fall.cpp — m of fall at a world position. Integrates to `volume` over the plane.
double fallThickness(const Fall& fall, Vec2 at);
// The parts of that law that hold over the whole fall, worked out once for a fall read at many points.
struct FallLaw {
    double peak{};    // 1/m², the blanket's thickness per unit volume at the vent, before the hollow
    double whole{};   // m², what the hollow over the vent leaves of the blanket's spread
};
FallLaw fallLaw(const Fall& fall);
double fallThickness(const Fall& fall, const FallLaw& law, Vec2 at);
// The cone of an eruption: fragments fall around the vent by the law above, summed over each cell
// so that no volume is lost to the grid, and slide to rest at `reposeTan`.
void buildCone(Emplacement& field, const Fall& fall, float reposeTan);
// The fall of fragments that belongs to a deposit of `volume` m³: as wide as that volume at rest.
Fall coneFall(Vec2 vent, double volume);
// The blanket of ash of `volume` m³ under a wind in m/s.
Fall ashFall(Vec2 vent, double volume, Vec2 wind);

// Collapse.cpp — m by which each cell drops when `volume` m³ leave the chamber under `centre` at
// once. `blockDrop` is the roof: a block inside a ring fault, with a wall as narrow as the grid can
// draw, the same depth everywhere inside; the layering under the pit goes down by it. `founder` is
// what the surface of `field` drops by over that block: to a level floor, which truncates a cone.
std::vector<float> blockDrop(const Emplacement& field, Vec2 centre, double volume, Magma magma);
std::vector<float> founder(const Emplacement& field, const std::vector<float>& block);
double calderaRadius(double volume, Magma magma);

} // namespace derivean::sim
