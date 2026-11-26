#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>

namespace derivean::sim {

// Noise only supplies seeded heterogeneity (initial roughness, rock quality, facies boundaries).
// A positive period makes the lattice wrap after that many lattice cells on both axes.
double gradientNoise(uint64_t seed, double x, double z, int period = 0);
double fractalNoise(uint64_t seed, double x, double z, int octaves, double gain = 0.5, int period = 0);
double ridgedNoise(uint64_t seed, double x, double z, int octaves, double gain = 0.5, int period = 0);
double valueNoise3(uint64_t seed, double x, double y, double z);

struct CellularSite {
    double nearest{}, second{};   // distances in lattice units
    int64_t cellX{}, cellZ{};     // lattice cell that owns the nearest site
    Vec2 site{};                  // position of the nearest site in lattice units
    int64_t otherX{}, otherZ{};   // lattice cell of the second nearest site
    Vec2 other{};
};

CellularSite cellularNoise(uint64_t seed, double x, double z, double jitter = 0.9, int period = 0);

} // namespace derivean::sim
