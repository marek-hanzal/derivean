#pragma once

#include "core/type/Vec.hpp"
#include "geometry/type/Vec3.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// What stands at the top of one cell, as a picture needs it. Colours are linear, 0..1.
struct SceneCell {
    float top{};        // m: ground, ice or standing water, whichever is highest
    float r{}, g{}, b{};
    float gloss{};      // 1 on open water, 0 on ground
};

// A square grid of cells of one level. Cells no tile was built for hold the next coarser layer's
// surface with `weight` 0; towards such cells the weight of built ones falls off, so two levels
// that disagree by the relief of the finer one meet without a wall.
struct SceneLayer {
    double cell{};      // m
    Vec2 origin;        // world position of the centre of cell (0, 0)
    int width{}, height{};
    std::vector<SceneCell> cells;
    std::vector<float> weight;   // 0..1

    size_t index(int x, int z) const { return size_t(z) * size_t(width) + size_t(x); }
};

struct Scene {
    std::vector<SceneLayer> layers;   // finest first
    float lowest{}, highest{};        // m, over every layer
    double seaLevel{};
};

// Angles in radians. Heading 0 looks north (-Z) and grows turning east; pitch is positive upwards.
struct Camera {
    Vec3 eye;
    double heading{}, pitch{};
    double fov{1.05};          // horizontal
    int width{1280}, height{800};
};

struct Light {
    double azimuth{};          // compass direction the sun stands in
    double elevation{0.6};
    double visibility{90e3};   // m at which haze has taken about two thirds of the contrast
};

} // namespace derivean::sim
