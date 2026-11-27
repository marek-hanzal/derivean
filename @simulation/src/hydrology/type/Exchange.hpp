#pragma once

#include "hydrology/type/Ocean.hpp"

namespace derivean::sim {

// Straits between the same two cells with the same bed act as one throat of their summed area.
struct Throat {
    Strait way;                    // `way.a` is the `a` of the first of them; `way.sill` the effective
                                   // sill, max(sill, ground at a, ground at b)
    std::vector<size_t> straits;   // indices into Ocean::straits
};

// One debit and its credit: what left one basin and arrived in another.
struct Transfer {
    int32_t from{}, to{};   // basins
    Tally carried;
};

// One integration of the straits, from the moment it starts moving water until the bodies are
// written back to the cells. Each lake is mixed, so what leaves it carries its strength and what
// stays keeps it; what arrives mixes in at once.
struct Exchange {
    std::vector<Tally> lake;             // by topmost basin: what that lake holds now
    std::vector<int32_t> lakeAtStart;    // Lakes::lakeOf when it began
    std::vector<uint8_t> touched;        // by basin: it was a lake that gave or received
    std::vector<Transfer> transfers;
};

} // namespace derivean::sim
