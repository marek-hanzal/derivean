#pragma once

#include "planet/type/Planet.hpp"

namespace derivean::sim {

// Heat reaching the surface from below, as the planet's history left it. The conductive part
// follows from the kind and age of the crust; the magmatic part is melt the planet recorded rising
// into the column and still cooling there. Melt does not rise evenly under a 65 km cell: it gathers
// into centres whose places are seeded in the frame of the crust, so they travel with their plate.
struct Geotherm {
    float gradient{};    // K/m near the surface
    float magmatic{};    // 0..1, share of the gradient owed to melt still cooling under the point
    float sinceMelt{};   // yr since melt last rose under the point; beyond the age of the body where it never did
    float centre{};      // 0..1, closeness to the nearest magmatic centre of the province
};

Geotherm geothermAt(const Planet& planet, Vec2 position);

// The magmatic centre `Geotherm::centre` is measured against: where melt gathers under a province.
// A centre lives for one epoch of `CentreLife` (1.5 Myr); the next epoch draws a new lattice, so a
// province's melt wanders between places instead of feeding one for ever. Its lattice cell and epoch
// are its identity. The position is exact while the point and the centre stand on the same column of
// crust (`crustFrame`). `geothermAt` uses the epoch of the planet's present.
struct MagmaticCentre {
    Vec2 position;             // m, where it is now
    int64_t cellX{}, cellZ{};  // lattice cell in the frame of the crust
    int64_t epoch{};           // floor(time / CentreLife)
    float vigor{};             // 0.35..1, its share of the province's melt
};

MagmaticCentre centreNear(const Planet& planet, Vec2 position, double time);

// Rock temperature in °C at a depth below a surface of the given annual mean.
inline float rockTemperature(const Geotherm& geotherm, float surface, float depth) {
    return surface + geotherm.gradient * std::max(0.0f, depth);
}

} // namespace derivean::sim
