#pragma once

#include "core/type/Field.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

struct Rgb {
    uint8_t r{}, g{}, b{};
};

// Height tint against the local water level: blues below, green through brown to white above.
Rgb hypsometricTint(double aboveWater);

// Lambert shade of a height grid lit from the north-west, in [0.35, 1.25].
double hillshade(const Field<float>& elevation, int x, int z, double cellSize, double exaggeration = 1.0);

// Tinted, shaded map of a height grid; one pixel per cell. `water` may hold a per-cell water
// surface (lakes, rivers) and falls back to the uniform sea level when empty.
std::vector<uint8_t> reliefImage(const Field<float>& elevation, double seaLevel, double cellSize,
                                 double exaggeration = 1.0, const Field<float>* water = nullptr);

void putPixel(std::vector<uint8_t>& image, int width, int x, int z, Rgb color);

} // namespace derivean::sim
