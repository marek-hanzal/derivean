#pragma once

#include "groundwater/type/Aquifer.hpp"

namespace derivean::sim {

enum class Outlet : uint8_t {
    None,
    Rising,     // lower storey: where its water stands at the ground, or in a stream, lake or the sea
    Lip,        // upper storey: where the tight bed it runs on comes to daylight
    Underflow,  // no way to the surface inside the tile: the water goes on below, out of the model
};

// How the soluble rock of a tile's own cells drains. Nodes are the tile's own nodes, two per own
// cell (upper storey first), row-major; entries of nodes that are not karst stay at their defaults.
struct KarstNet {
    std::vector<int32_t> parent;    // node the water of a node goes on to; -1 at an outlet and for nodes outside the net
    std::vector<uint8_t> toward;    // neighbour direction of that node; 255 where there is none
    std::vector<Outlet> outlet;
    std::vector<int32_t> root;      // outlet node a node drains to; -1 outside the net
    std::vector<double> flux;       // m³/yr through each node, its own share included
    std::vector<float> run;         // m along the net from a node to its outlet
    std::vector<int32_t> order;     // karst nodes, farthest from their outlets first
};

// Karst.cpp — a few rounds of path finding in which every used path becomes easier to follow in
// proportion to the water it carried, so flow gathers into a few conduits and the largest draws
// in its neighbours.
KarstNet drainKarst(const WaterField& field);

// Whether the step from an own cell towards neighbour k runs along a joint plane both cells lie on:
// 2 for a zone, 1 for a corridor, 0 for neither. `set` receives the joint set.
int planeAlong(const WaterField& field, size_t from, size_t to, int k, int& set);

} // namespace derivean::sim
