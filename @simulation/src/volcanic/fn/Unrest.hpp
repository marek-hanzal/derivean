#pragma once

#include "volcanic/type/Pile.hpp"

namespace derivean::sim {

struct World;

// A vent at one moment, as its chamber stands: what a dormant volcano gives away before it erupts.
// A function of the seed, the vent and the date alone: asking twice, walking there or jumping a
// century gives the same answer.
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

// Unrest.cpp — `time` in years of the era: the world's age plus what the calendar has added.
Unrest ventUnrest(uint64_t seed, const Vent& vent, double time);
// m of that uplift at a distance from the vent: a small source swelling at the chamber's depth.
double upliftAt(const Unrest& unrest, double distance);

// What stands at a place and why: the vent whose conduit lies nearest, its history and its state,
// and the pile under the place. Builds the region tile there and blocks until it is finished.
struct VolcanicRecord {
    bool found{};            // a vent lies within reach
    Vent vent;
    Vec2 site;               // m, its youngest conduit
    double distance{};       // m from the place to it
    Unrest unrest;
    PileColumn pile;         // of the region's cell at the place
    Material rock{Material::Air};   // what the top of that pile is; Air where there is none
};

VolcanicRecord volcanicAt(World& world, Vec2 position, double calendarYears, double reach = 60000.0);

} // namespace derivean::sim
