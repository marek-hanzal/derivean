#pragma once

#include "landscape/type/Level.hpp"

#include <vector>

namespace derivean::sim {

// The regional flow of one region tile: ground water that goes down below what a local tile can
// follow (WaterReach) and comes up tens of kilometres away, where the region's valleys, lakes and
// coasts cut down to it or where its table meets the ground. One storey, the region's cells. A
// pure function of the setting and the key: it reads the finished region tiles around it and no
// other tile's ground water (groundwater/fn/Regional.hpp).
struct RegionalBudget {
    double recharge{};    // m³/yr of the tile's own cells going down into the regional flow
    double risen{};       // of it, coming up in the tile's own cells
    double exported{};    // of it, coming up beyond the tile's edge
    double lag{};         // of it, reaching nothing the field holds: farther than the margin reads
    double arriving{};    // m³/yr coming up in the tile's own cells, from wherever in the field it went down
};

struct RegionalTile {
    TileKey key;                  // Level::Region
    std::vector<float> recharge;  // m³/yr going down into the regional flow, per own cell, row-major
    std::vector<float> upwelling; // m³/yr coming up, per own cell
    std::vector<float> head;      // m, where the regional water stands: at most the ground, except under a tight bed;
                                  // infinity under a tight bed with no way out in the field
    std::vector<uint8_t> outlet;  // 1 a river, 2 a lake, 3 the sea, 4 the table meets the ground; 0 none
    // Of the water coming up, by its flow: where it went down and how far it came.
    std::vector<float> rate;      // m/yr of recharge where it went down
    std::vector<float> high;      // m, ground where it went down
    std::vector<float> far;       // m it travelled
    std::vector<float> deep;      // m, depth of the base of circulation where it went down: as deep as it went
    RegionalBudget budget;
};

} // namespace derivean::sim
