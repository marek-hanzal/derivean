#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace derivean::sim {

// What soluble rock does to a landscape's surface water during a replay. Water that goes down in
// a cell of a soluble body comes out at the lowest cell where that body meets other rock, the sea
// or the domain's rim: its rising. It takes one step underground. Every array has one entry per
// cell of the domain once a step has met soluble rock, and none before (groundwater/fn/Sinking.hpp).
struct SinkWork {
    std::vector<float> rate;       // m/yr a wetted square metre takes in; 0 off soluble rock and near its rising
    std::vector<float> soak;       // 0..1 of the runoff that soaks in where it falls (groundwater/fn/Recharge.hpp); 0 where `rate` is
    std::vector<int32_t> rising;   // cell the water sinking here in this step comes out at in the next; -1 where none sinks
    std::vector<float> received;   // m³/yr an open hollow took in; -1 where the cell is no open hollow this step
    std::vector<float> sunk;       // m³/yr gone underground in each cell this step: soaked in and taken from streams
    std::vector<float> risen;      // m³/yr coming out at each cell this step: what sank in the step before
    std::vector<int32_t> body, members, outlet;   // scratch of the labelling
    std::vector<float> lake;                       // scratch: m³/yr a lake's floor took in the step before, at its outlet
    std::vector<float> ponded;     // m³/yr an open hollow could not take this step: it runs over the hollow's pass in the same step
    double total{};                // m³/yr gone down this step, over the domain
    // Spilling over the passes of open hollows that could not take their water (groundwater/fn/Spill.hpp).
    std::vector<float> extra;                   // scratch: m³/yr still to pass on from each cell
    std::vector<int32_t> lakeOf, pour;          // scratch: the lake whose catchment holds a cell; the cell each lake runs over into (fn/Spill.cpp)
    std::vector<std::vector<int32_t>> lakes;    // scratch: the cells of each lake's catchment
    std::vector<std::pair<int32_t, int32_t>> due;   // scratch: heap of the cells with water to pass on, by rank (fn/Spill.cpp)
    double spilled{}, overEdge{};               // m³/yr this step: run over passes (once per pass); left over the domain's edge or into a finished neighbour's cells

    bool active() const { return !rate.empty(); }
};

} // namespace derivean::sim
