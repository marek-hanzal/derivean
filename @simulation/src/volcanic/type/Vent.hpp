#pragma once

#include "core/type/Vec.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// What the melt of a province is. It decides how its lava runs and how much of it leaves as ash.
enum class Magma : uint8_t {
    Basalt,     // fluid melt of rifts and plumes
    Andesite,   // viscous, gas-rich melt above a sinking plate
};

// What a vent erupts during one step of a region's replay: its single eruptions of that time added up.
struct Episode {
    double time{};       // yr, start of the step
    double volume{};     // m³ of rock that reaches the surface
    double paroxysm{};   // m³ of the largest single eruption among them
    Vec2 site;           // m, where the conduit reaches the surface
};

// One eruption of a vent's own sequence.
struct Eruption {
    double time{};       // yr
    double volume{};     // m³
};

// A conduit that fed one edifice: the plug it leaves stands as a neck once the cone is gone.
struct Feeder {
    Vec2 site;
    float radius{};      // m
    double first{}, last{};   // yr it fed eruptions
    double volume{};     // m³ it brought up
};

// One magmatic centre of the planet and everything it erupts. Its history is a function of the
// seed and the planet's record alone, so every tile and every date sees the same vent.
struct Vent {
    uint64_t id{};
    Vec2 centre;         // m, world position of the magmatic centre
    Magma magma{Magma::Basalt};
    float vigor{};       // 0..1, share of its province's melt that gathers here
    double begin{};      // yr the history below starts at: the start of the region's window
    double until{};      // yr it is recorded to: the end of that window
    double supply{};     // m³/yr the centre still supplies at that end; 0 once the melt has stopped rising
    double erupted{};    // m³ over the region's window
    double held{};       // m³ in the chamber at `until`: supplied since the last eruption
    int64_t count{};     // eruptions so far: the number of the next one in the vent's sequence
    std::vector<Eruption> recent;    // the last few before `until`, oldest first
    std::vector<Episode> episodes;   // by time; steps without an eruption are left out
    std::vector<Feeder> feeders;   // by time of first use
};

} // namespace derivean::sim
