#pragma once

#include "groundwater/type/Aquifer.hpp"

namespace derivean::sim {

enum class Outlet : uint8_t {
    None,
    Rising,   // the water stands at the ground of the cell, or in the stream, lake or sea that lies in it
    Lip,      // upper storey: the tight bed the water runs on comes to daylight beside the cell
};

// How the soluble rock of a tile's own cells drains. Entries are per node of the field, two per
// cell with the upper storey first; nodes that are not karst keep their defaults.
struct KarstNet {
    std::vector<int32_t> parent;    // node the water of a node goes on to; -1 at an outlet and outside the net
    std::vector<uint8_t> toward;    // neighbour direction of that node; 255 where there is none
    std::vector<Outlet> outlet;
    std::vector<int32_t> root;      // outlet node a node drains to; -1 outside the net
    std::vector<double> flux;       // m³/yr through each node, what it took in itself included
    std::vector<float> head;        // m, level the water stands at in the node's conduit
    std::vector<float> run;         // m along the net from a node to its outlet
    std::vector<int32_t> order;     // karst nodes, most steps from their outlet first: every node before the one it drains to
};

// Karst.cpp — where the water of each karst node goes. Every node takes the way that lets its water
// stand lowest: out at its own ground, at a lip or into open water where it has one, or on to a
// neighbour at the cost of the head the step uses up. Unbroken rock uses much, a joint corridor
// less, a joint zone least, and a step that carried water in the round before less again in
// proportion to that water. Over a few rounds flow gathers into few conduits and the outlets that
// drew most draw their neighbours' water too. The tile is closed: no step crosses its edge.
KarstNet drainKarst(const WaterField& field);

// Whether the step from a cell towards neighbour k runs along a joint plane both cells lie on:
// 2 for a zone, 1 for a corridor, 0 for neither. `set` receives the joint set.
int planeAlong(const WaterField& field, size_t from, size_t to, int k, int& set);

} // namespace derivean::sim
