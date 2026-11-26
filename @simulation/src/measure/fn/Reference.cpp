#include "measure/fn/Reference.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "landscape/type/Terrain.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;

float white(uint64_t seed, int x, int z) { return float(2.0 * unit(seed, x, z) - 1.0); }

} // namespace

Field<float> fractalRelief(uint64_t seed, int side, double wavelength, double height, double gain) {
    Field<float> surface(side, side);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) surface.at(x, z) = float(height * fractalNoise(seed, x / wavelength, z / wavelength, 7, gain));
    return surface;
}

Field<float> combRelief(uint64_t seed, int side, double spacing, double depth, double noise) {
    Field<float> surface(side, side);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x)
            surface.at(x, z) = float(0.5 * (side - x) + 0.5 * depth * std::cos(2 * Pi * z / spacing) + noise * white(seed, x, z));
    return surface;
}

Field<float> sawRelief(uint64_t seed, int side, double spacing, double height, double noise) {
    Field<float> surface(side, side);
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const double phase = (0.8 * x + 0.6 * z) / spacing;
            const double tooth = 1.0 - 2.0 * std::abs(phase - std::floor(phase) - 0.5);
            surface.at(x, z) = float(height * tooth + noise * white(seed, x, z));
        }
    return surface;
}

Mosaic coneMosaic(uint64_t seed, int side, bool locked) {
    Mosaic mosaic;
    mosaic.tiles = side / TileCells;
    mosaic.cell = 1;
    mosaic.seaLevel = -1e9;
    mosaic.ground = Field<float>(side, side);
    mosaic.water = Field<float>(side, side);
    mosaic.discharge = Field<float>(side, side);
    mosaic.flowTo = Field<uint8_t>(side, side, uint8_t(255));
    const double middle = 0.5 * (side - 1);
    const auto height = [&](int x, int z) { return std::sqrt((x - middle) * (x - middle) + (z - middle) * (z - middle)); };
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            mosaic.ground.at(x, z) = float(height(x, z));
            mosaic.water.at(x, z) = float(height(x, z) - 1);
            // Only the inscribed disc carries water: the corners of the square would count the diagonals twice.
            mosaic.discharge.at(x, z) = height(x, z) < middle ? 1.0f : 0.0f;
            if (x == 0 || z == 0 || x == side - 1 || z == side - 1) continue;
            int best = -1, second = -1;
            double slope[8];
            for (int k = 0; k < 8; ++k) {
                slope[k] = (height(x, z) - height(x + NeighborX[k], z + NeighborZ[k])) / NeighborReach[k];
                if (best < 0 || slope[k] > slope[best]) second = best, best = k;
                else if (second < 0 || slope[k] > slope[second]) second = k;
            }
            if (slope[best] <= 0) continue;
            // Between an axis and a diagonal neighbour the fall line lies at atan of the diagonal's share
            // of steps. Even octants start at an axis, odd ones at a diagonal.
            const double heading = std::atan2(middle - z, middle - x), octant = heading / (Pi / 4);
            const double within = (octant - std::floor(octant)) * (Pi / 4);
            const double diagonalShare = std::tan((int(std::floor(octant)) & 1) ? Pi / 4 - within : within);
            const bool swap = !locked && slope[second] > 0 && unit(seed, x, z) < (best % 2 == 0 ? diagonalShare : 1.0 - diagonalShare);
            mosaic.flowTo.at(x, z) = uint8_t(swap ? second : best);
        }
    }
    return mosaic;
}

} // namespace derivean::sim
