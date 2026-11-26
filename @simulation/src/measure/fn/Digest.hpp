#pragma once

#include "landscape/type/Terrain.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {

// Fingerprints of the state a world must reproduce bit for bit, comparable across runs and processes.
struct TileDigest {
    uint64_t ground{}, water{}, discharge{};
    bool operator==(const TileDigest&) const = default;
};

TileDigest digestOf(const Tile& tile);
uint64_t digestOf(const Planet& planet);
uint64_t digestOf(const void* data, size_t bytes, uint64_t seed = 0);

// Cells in which two builds of the same tile differ in ground, water or discharge, and by how much.
struct TileDifference {
    size_t cells{};
    double ground{};      // m, largest difference of the ground surface
    double water{};       // m
    double discharge{};   // largest relative difference
};

TileDifference differenceOf(const Tile& a, const Tile& b);

} // namespace derivean::sim
