#pragma once

#include "core/type/Vec.hpp"
#include "ground/type/Parting.hpp"

#include <vector>

namespace derivean::sim {

// A height field blocks run over, with what its surface is made of. Row-major, `width * height` cells.
struct FallGrid {
    Vec2 origin;                    // world position of the outer corner of cell (0, 0)
    double cell{};                  // m
    int width{}, height{};
    std::vector<float> ground;      // m, at cell centres
    std::vector<float> roughness;   // m, size of the pieces the surface is made of
    std::vector<float> drag;        // friction the ground adds by yielding: soft soil a little, standing water a lot

    bool contains(Vec2 p) const;    // inside the centres of the outer cells, where the surface interpolates
    float heightAt(Vec2 p) const;
    Vec2 gradientAt(Vec2 p) const;
    size_t cellAt(Vec2 p) const;
};

// A face that is breaking up.
struct FallSource {
    Vec2 at;              // m
    float volume{};       // m³ of rock it releases
    BlockHabit habit;     // what its rock breaks into
    Material rock{Material::Bedrock};
    uint64_t id{};        // stable for a world: its blocks are numbered under it
};

struct FallenBlock {
    Vec2 rest, source;
    float size{};         // m, intermediate axis
    float flatness{};     // thickness over width
    float travel{};       // m along its path
    float heading{};      // rad, direction it was moving in when it stopped
    Material rock{Material::Bedrock};
    uint64_t draw{};      // seeded bits for its shape and attitude
};

} // namespace derivean::sim
