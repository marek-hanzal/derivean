#pragma once

#include "hydrology/type/Ocean.hpp"

namespace derivean::sim {

// Straits between the same two cells with the same bed act as one throat of their summed area.
struct Throat {
    Strait way;                    // `way.a` is the `a` of the first of them; `way.sill` the effective
                                   // sill, max(sill, ground at a, ground at b)
    std::vector<size_t> straits;   // indices into Ocean::straits
};

// One integration of the straits, from the moment it starts moving water until the bodies are
// written back to the cells. Each lake is mixed, so what leaves it carries its strength and what
// stays keeps it; what arrives mixes in at once. Bodies are known by their topmost basin, as the
// lakes stand at the time.
struct Exchange {
    std::vector<Tally> lake;             // by topmost basin: what that lake holds now
    std::vector<Tally> start;            // by topmost basin: what it held when the integration began
    std::vector<Tally> account;          // by basin: what entered the lake it tops, less what left:
                                         // segments, crust it dissolved, and what it took over or
                                         // gave up when lakes met or parted
    std::vector<Tally> turnover;         // by basin: the same, summed without sign
    std::vector<int32_t> lakeAtStart;    // Lakes::lakeOf when it began
    std::vector<int32_t> scanned;        // by basin: how many of its members, by rising height, have
                                         // been under water since it began and so hold no crust
    std::vector<uint8_t> touched;        // by basin: it was a lake that took part
    size_t segments{};                   // that moved water
};

} // namespace derivean::sim
