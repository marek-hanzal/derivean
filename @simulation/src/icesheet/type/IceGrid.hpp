#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace derivean::sim {

// Ice on a square grid with the climate that feeds it: what the shallow-ice relaxation works on.
// The same grid serves the planet (65 km cells, wrapping) and one region domain (4 km cells, held
// along its rim). Arrays are row-major with width · height entries.
struct IceGrid {
    int width{}, height{};
    double cell{};                    // m
    bool wraps{};                     // the planet repeats; any other grid ends at its edge
    float seaLevel{};                 // m, of the climate solved for
    float cooling{};                  // °C against the present, zero or negative

    // Given.
    std::vector<float> bed;           // m, ground with no ice on it
    std::vector<float> warmth;        // °C, present annual mean reduced to sea level
    std::vector<float> halfRange;     // °C between the annual mean and the warmest month
    std::vector<float> rain;          // m/yr, present precipitation
    std::vector<float> heat;          // K/m, temperature gradient in the rock under the ice
    std::vector<uint8_t> held;        // 1 where the thickness is given and does not change

    // Found.
    std::vector<float> thickness;     // m
    std::vector<float> sink;          // m the load has pressed the crust down
    std::vector<float> balance;       // m of ice per year the climate adds at the surface, negative where it melts
    std::vector<float> thaw;          // 0 frozen to the bed .. 1 at the melting point

    size_t index(int x, int z) const { return size_t(z) * size_t(width) + size_t(x); }
    size_t cells() const { return size_t(width) * size_t(height); }
    // m, top of the ice, or the ground where there is none.
    float surface(size_t i) const { return bed[i] - sink[i] + thickness[i]; }
};

} // namespace derivean::sim
