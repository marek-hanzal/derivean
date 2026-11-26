#pragma once

#include <cstdint>
#include <vector>

namespace derivean::sim {

// A depression of the solid surface: the cells that drain to one pit, or two such depressions and
// the ground that joins them above the saddle they share. Basin 0 is the outside of the grid.
struct Basin {
    int32_t parent{-1};     // 0 when it overflows across the rim; -1 while nothing higher closes it
    int32_t sibling{-1};
    int32_t child{-1};      // one of its two children, the other being that one's sibling; -1 for a leaf
    int32_t over{};         // leaf basin its overflow runs into; 0 when that leaves the grid
    int32_t saddle{-1};     // cell it overflows across
    int32_t entry{-1};      // first cell beyond the saddle on the way down
    int32_t first{}, count{};   // its own cells in BasinTree::members, by rising height
    int32_t below{};        // cells of its children; all of them lie under `base`
    double base{};          // m, the pit of a leaf or the saddle its children met at
    double spill{};         // m, level at which it overflows
    double capacity{};      // m³ it holds between base and spill
    int32_t feeder{-1};     // rim cell whose standing water it overflows against and is kept full by; -1 for none
};

// The depressions of one surface, nested. It depends on the solid alone and stays valid until the
// ground changes.
struct BasinTree {
    double area{};                    // m² of one cell
    std::vector<Basin> basins;        // children before parents
    std::vector<float> height;        // m, ground of Open cells and the held surface of Rim cells
    std::vector<int32_t> order;       // cells by rising (height, index)
    std::vector<int32_t> members;
    std::vector<int32_t> basinOf;     // basin a cell joined; 0 where the ground falls to the rim
    std::vector<int32_t> receiver;    // steepest lower neighbour; -1 for a pit
    std::vector<int32_t> leafOf;      // leaf basin water on the cell runs down to; 0 when it runs out
};

// How full the depressions are. A basin holds water of its own only when its children are full.
struct Lakes {
    std::vector<double> own;        // m³ above each basin's base
    std::vector<uint8_t> full;      // the basin and everything under it is at its sill
    std::vector<double> level;      // m, surface of the lake the basin lies in
    std::vector<int32_t> lakeOf;    // topmost basin under the same surface
    std::vector<int32_t> ranked;    // those topmost basins, each before the lake it overflows into
};

} // namespace derivean::sim
