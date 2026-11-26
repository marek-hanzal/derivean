#pragma once

#include "core/type/Vec.hpp"

#include <vector>

namespace derivean::sim {

// What a field of dunes looks like in numbers. Slopes are rise over run on cells with sand.
struct DuneStats {
    double sandShare{};      // share of the cells that carry at least a decimetre of loose sand
    double meanSand{};       // m over all cells
    double slipShare{};      // share of the sandy cells within a tenth of repose
    double steepest{};
    double windward{}, lee{};   // mean slope of sandy ground that rises with the wind, and that falls with it
    double leeSlipShare{};   // share of the slip-face cells that face away from the wind
    double crestAngle{};     // degrees between the crest lines and the wind: 90 across it, 0 along it
    double order{};          // 0 when slopes face every way alike, 1 when all crests run one way
    double height{}, tallest{};   // m from trough to crest across the crests: median and largest
    double spacing{};        // m from crest to crest across the crests, median
    int crests{};
    int patches{};           // separate bodies of sand
};

// `top` and `sand` are row-major squares of `size` cells; `wind` is where sand goes on balance.
DuneStats measureDunes(int size, double cell, const std::vector<float>& top, const std::vector<float>& sand, Vec2 wind, float reposeTan);

} // namespace derivean::sim
