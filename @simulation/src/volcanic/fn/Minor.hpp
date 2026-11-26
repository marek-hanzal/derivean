#pragma once

#include "volcanic/constant/Rheology.hpp"

namespace derivean::sim {

// An eruption too small for a region's cells: a cinder cone with its flow, a dome, a summit cone.
// The region leaves their share of a vent's young episodes out, and the local level builds them.
struct MinorEvent {
    double time{};       // yr
    Vec2 site;
    double fall{};       // m³ of fragments around the vent
    double lava{};       // m³
    Rheology rheology;   // of that lava
};

// Share of an episode that comes up in such eruptions; none of an episode older than the local
// level's settling time, whose small cones no longer stand.
double minorShare(const Vent& vent, const Episode& episode);

// The minor eruptions of a vent dated after `from` and up to `to`, in order of time.
void minorEvents(uint64_t seed, const Vent& vent, double from, double to, std::vector<MinorEvent>& into);

} // namespace derivean::sim
