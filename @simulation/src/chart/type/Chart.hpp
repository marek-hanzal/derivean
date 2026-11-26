#pragma once

#include "core/type/Vec.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace derivean::sim {

// A square of the world to read, north up. Any position is valid: the planet repeats under it.
struct ChartRequest {
    Vec2 centre;     // m
    double span{};   // m along each edge
    int size{};      // pixels along each edge
};

enum class ChartWater : uint8_t { Dry, Sea, Lake, Stream };

// What the simulation holds over that square. Rasters have size² entries, row-major with Z as the
// row: pixel (i, j) is centred at origin + (i + 0.5, j + 0.5) · pitch.
struct Chart {
    Vec2 origin;       // m, north-west corner
    double pitch{};    // m per pixel
    int size{};
    size_t tiles{};    // finished tiles the atlas held when the chart was read
    uint8_t finest{};  // finest Level a chart of this pitch reads; 0 when a pixel is too wide for any tile
    std::vector<float> ground;      // m, top of loose or solid ground
    std::vector<float> water;       // m, surface of the sea, a lake or a stream; NaN over dry ground
    std::vector<float> discharge;   // m³/yr, the largest mean annual flow crossing the pixel; none where only the planet is known
    std::vector<float> ice;         // m of glacier ice
    std::vector<float> warmth;      // °C, annual mean at the ground
    std::vector<float> cover;       // m of loose material on the rock
    std::vector<uint8_t> bedrock;   // Material of that rock; Bedrock where only the planet is known
    std::vector<uint8_t> waters;    // ChartWater at the pixel centre
    std::vector<uint8_t> level;     // Level the pixel was read from; 0 where only the planet is known
};

} // namespace derivean::sim
