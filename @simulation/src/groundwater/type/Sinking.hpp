#pragma once

#include <cstdint>
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
    std::vector<float> ponded;     // m³/yr an open hollow could not take this step: it leaves the step's surface water there
    double total{};                // m³/yr gone down this step, over the domain

    bool active() const { return !rate.empty(); }
};

} // namespace derivean::sim
