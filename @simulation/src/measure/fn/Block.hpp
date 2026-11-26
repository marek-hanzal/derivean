#pragma once

#include "landscape/fn/Process.hpp"

namespace derivean::sim {

// A plateau of known rock with a river held at its foot: the ground on which what a wall may do is
// known beforehand. The river runs east along the middle of the domain between two rim cells that
// stay at its level; everything else starts `height` metres above it and is left to the level's
// own slope and river processes.
struct BlockSpec {
    Level level{Level::Site};
    Material rock{Material::Granite};   // the block, or the strong member of its beds
    Material weak{Material::Air};       // Air for one rock throughout; otherwise beds alternate with this one
    double bedThickness{16};            // m of every bed
    double sound{1};                    // cohesion of the rock mass against what its kind usually has: above one for massive rock
    double dip{0};                      // rise over run with which the beds fall towards the east
    double height{400};                 // m from the river to the plateau
    int floorHalf{3};                   // cells from the middle line to the foot of the walls
    bool river{true};                   // false leaves the block to its slopes alone: nothing is cut or carried away
    double rain{1.0};                   // m/yr
    double warmth{10};                  // °C at the ground
    double riverFlow{3e8};              // m³/yr that enters with the river
    double uplift{0};                   // m/yr the block rises against the river
    bool led{false};                    // hold the averaged surface on the starting one, as a tile under a parent is held
    int steps{0};                       // 0 runs the settling time of the level
    uint64_t seed{42};
};

Domain blockDomain(const BlockSpec& spec);

// One step of the level's processes on the block; `scratch` lives as long as the replay.
void stepBlock(Domain& domain, FlowScratch& scratch, const BlockSpec& spec);

// The block after `steps`; flow fields describe the last step.
Domain replayBlock(const BlockSpec& spec);

// m, tallest run of ground along a line across the river in which every cell falls by more than
// `rise` over run to the next: the height of the highest face that steep.
double tallestFace(const Domain& domain, double rise);

} // namespace derivean::sim
