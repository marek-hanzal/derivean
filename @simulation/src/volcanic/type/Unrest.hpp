#pragma once

#include "volcanic/type/Pile.hpp"

namespace derivean::sim {

// A vent at one moment, as its chamber stands: what a dormant volcano gives away before it erupts.
struct Unrest {
    bool supplied{};       // melt still rises under it; everything below is zero for a vent that has gone out
    double fill{};         // 0..1, share of its next eruption that the chamber has gathered
    double held{};         // m³ in the chamber
    double sinceLast{};    // yr since it last erupted
    double lastVolume{};   // m³ of that eruption
    double untilNext{};    // yr until it erupts
    double nextVolume{};   // m³ it will bring up
    double uplift{};       // m the ground right over the chamber has risen since the last eruption
    double quakes{};       // small earthquakes a year around the chamber
    double depth{};        // m from the ground to the chamber
};

// What stands at a place and why: the vent whose conduit lies nearest, its history and its state,
// and the pile under the place.
struct VolcanicRecord {
    bool found{};            // a vent lies within reach
    Vent vent;
    Vec2 site;               // m, its youngest conduit
    double distance{};       // m from the place to it
    Unrest unrest;
    PileColumn pile;         // of the region's cell at the place
    Material rock{Material::Air};   // what the top of that pile is; Air where there is none
};

} // namespace derivean::sim
