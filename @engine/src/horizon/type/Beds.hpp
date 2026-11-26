#pragma once

#include "simulation/type/World.hpp"
#include "strata/type/Column.hpp"

namespace derivean {

// The layering under one point as the simulation's voxels read it: which bed a height lies in.
// It is the planet's own column there, so it needs no tile; the voxels blend the thickness of the
// columns around them, which moves a contact by less than a far cube.
struct Beds {
    uint64_t seed{};
    sim::StrataColumn column;
    float fold{};
    float datum{};     // m, the height depths in the layering are measured down from
    sim::Bed bed;      // the bed last asked for
};

Beds bedsAt(const sim::World& world, sim::Vec2 position, float datum);

// The rock at height `y` metres.
sim::Material rockAt(Beds& beds, double y);

} // namespace derivean
