#pragma once

#include "core/type/Vec.hpp"
#include "landscape/type/Terrain.hpp"

#include <vector>

namespace derivean::sim {

// What the level above holds of the sheets, read on the cells of a finer domain: per domain cell,
// linearly between the parent's cell centres. Empty arrays and `any` false where no parent tile
// under the domain carries a record.
struct ParentSheet {
    bool any{};
    std::vector<float> iceNow, iceThen;     // m of ice over the parent's ground, now and at the last maximum
    std::vector<float> topNow, topThen;     // m, the parent's ice surface; its ground where it has none
    std::vector<float> slideX, slideZ;      // m/yr the last maximum's ice slid over the bed
    std::vector<float> worn;                // m of rock the parent's sheets ground off
    std::vector<float> esker;               // m of tunnel fill over the parent's cell
};

// A stream under the last maximum's ice that left a tunnel fill: one parent cell's stretch of it,
// from that cell's centre to the next one's.
struct Tunnel {
    Vec2 from, to;                  // world position
    double fill{};                  // m³ of gravel and sand along the stretch
    float discharge{};              // m³/yr of water through it
};

// The load the last sheet carries over a finer domain, m of material per cell by transport class.
struct SheetLoad {
    std::vector<float> load[SedimentClasses];
    // m the bed of a cell this domain does not evolve would have lost to the ice, for the shelter
    // it gives the cells down-ice; zero where it holds a finished neighbour's result already.
    std::vector<float> unstripped;
};

} // namespace derivean::sim
