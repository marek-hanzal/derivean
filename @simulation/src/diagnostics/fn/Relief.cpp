#include "diagnostics/fn/Relief.hpp"

#include <array>

namespace derivean::sim {
namespace {

struct Stop {
    double height;
    double r, g, b;
};

constexpr std::array<Stop, 13> Ramp{{
    {-7000, 8, 18, 60}, {-4000, 16, 44, 104}, {-1500, 30, 84, 150}, {-200, 70, 140, 190}, {-1, 126, 190, 214},
    {0, 108, 148, 96}, {150, 134, 164, 104}, {500, 176, 178, 118}, {1000, 186, 156, 106}, {1800, 150, 118, 92},
    {2800, 136, 124, 118}, {3800, 206, 204, 204}, {5500, 255, 255, 255},
}};

} // namespace

Rgb hypsometricTint(double aboveWater) {
    if (aboveWater <= Ramp.front().height) return {uint8_t(Ramp.front().r), uint8_t(Ramp.front().g), uint8_t(Ramp.front().b)};
    for (size_t i = 1; i < Ramp.size(); ++i) {
        if (aboveWater > Ramp[i].height) continue;
        const double t = (aboveWater - Ramp[i - 1].height) / (Ramp[i].height - Ramp[i - 1].height);
        return {uint8_t(mix(Ramp[i - 1].r, Ramp[i].r, t)), uint8_t(mix(Ramp[i - 1].g, Ramp[i].g, t)),
                uint8_t(mix(Ramp[i - 1].b, Ramp[i].b, t))};
    }
    return {255, 255, 255};
}

double hillshade(const Field<float>& elevation, int x, int z, double cellSize, double exaggeration) {
    const auto at = [&](int ix, int iz) {
        return double(elevation.at(std::clamp(ix, 0, elevation.width - 1), std::clamp(iz, 0, elevation.height - 1)));
    };
    const double dx = (at(x + 1, z) - at(x - 1, z)) / (2 * cellSize) * exaggeration;
    const double dz = (at(x, z + 1) - at(x, z - 1)) / (2 * cellSize) * exaggeration;
    // Surface normal (-dx, 1, -dz) against a light from the north-west at 40° elevation.
    const double lx = -0.542, ly = 0.643, lz = -0.542;
    const double lit = (-dx * lx + ly - dz * lz) / std::sqrt(dx * dx + 1 + dz * dz);
    return std::clamp(0.35 + 1.4 * (lit - 0.25), 0.35, 1.25);
}

void putPixel(std::vector<uint8_t>& image, int width, int x, int z, Rgb color) {
    const size_t at = (size_t(z) * size_t(width) + size_t(x)) * 3;
    image[at] = color.r;
    image[at + 1] = color.g;
    image[at + 2] = color.b;
}

std::vector<uint8_t> reliefImage(const Field<float>& elevation, double seaLevel, double cellSize,
                                 double exaggeration, const Field<float>* water) {
    std::vector<uint8_t> image(elevation.size() * 3);
    for (int z = 0; z < elevation.height; ++z) {
        for (int x = 0; x < elevation.width; ++x) {
            const double ground = elevation.at(x, z);
            const double level = water && water->at(x, z) > ground ? double(water->at(x, z)) : seaLevel;
            const bool flooded = level > ground;
            Rgb tint = hypsometricTint(ground - (flooded ? level : seaLevel));
            if (flooded && water && level > seaLevel + 0.5) tint = {64, 128, 182};
            const double shade = flooded ? 0.75 + 0.25 * hillshade(elevation, x, z, cellSize, exaggeration)
                                         : hillshade(elevation, x, z, cellSize, exaggeration);
            putPixel(image, elevation.width, x, z, {uint8_t(std::min(255.0, tint.r * shade)),
                                                    uint8_t(std::min(255.0, tint.g * shade)),
                                                    uint8_t(std::min(255.0, tint.b * shade))});
        }
    }
    return image;
}

} // namespace derivean::sim
