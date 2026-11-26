#pragma once

#include "world/type/VoxelWorld.hpp"

#include <optional>

namespace derivean {

inline constexpr int MaxBrushRadius = 13;
inline constexpr int DefaultBrushRadius = 7;

struct Command {
    double elapsed{};
    float x{}, z{};
    bool jump{}, sprint{}, active{};
    Position direction{};
    int radius{DefaultBrushRadius};
    BrushShape shape{BrushShape::Sphere};
    bool dig{}, place{};
    bool fly{};                          // toggles flight on the frame it is set
    bool boost{};                        // a sprint on foot, long-distance speed in flight
    float ascend{};                      // -1…1, vertical thrust in flight
    std::optional<Position> teleport;    // where to put the player's feet, m
    double timeRate{1};                  // calendar seconds per second of play; 0 holds the calendar still
    double timeJump{};                   // calendar seconds to skip at once
};

inline constexpr double MaxTimeRate = 365.0 * 86400.0;   // a year of calendar time per second of play

// The planet a game is played on.
struct PlanetOptions {
    uint64_t seed{42};
    double age{4.6e9};                              // yr
    std::optional<std::array<double, 2>> spawn;     // x and z in m; a shore near the origin when absent
    unsigned threads{};                             // 0 sizes the pools from the machine
    // The far terrain as levels of cubes instead of height-field patches. Both exist while the
    // renderer moves from one to the other; a game makes one of them.
    bool cubes{};
    int cubeReach{128};                             // cubes from the eye to the side of a level's box: a cube there is 1/reach rad wide
};

} // namespace derivean
