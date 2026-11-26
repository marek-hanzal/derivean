#pragma once

#include "material/constant/Materials.hpp"

namespace derivean::sim {

// Surface water on soluble, jointed rock goes underground. How fast is a matter of the rock alone,
// not of the water table under it, so the landscape can read it while it routes its water and a
// landscape tile stays a pure function of its key: the ground water model is worked out from
// finished tiles and takes what sank there as given.
struct SwallowBed {
    Material rock{Material::Bedrock};   // bed at the top of the solid rock
    float cover{};        // m of loose cover on it; alluvium plugs the holes
    float fracture{};     // 0..1, share of the cell in broken rock along joint planes (fractureShare)
    float openDepth{};    // m to which weather has opened the joints (RockMass::openDepth)
    float dip{};          // rad, of the layering: steep beds turn their partings to the rain
};

// m/yr a wetted square metre of such ground takes in; 0 for rock that water does not open.
float swallowRate(const SwallowBed& bed);

// m³/yr of a flow of `discharge` m³/yr that goes underground while it crosses a cell `cell` metres
// wide: the rate over the bed the water wets, a strip as wide as a stream of that flow and never
// wider than the cell. Never more than the flow: a small stream sinks whole, a river loses a share.
float swallowed(float rate, float discharge, float cell);

// m/yr by which sinking water lowers soluble ground, to be added to the lowering by runoff: the
// rock dissolved is that of `solubility` (m/yr per m/yr of water) for the water that goes down in
// the cell. Where water gathers and sinks the floor falls fastest, which gathers more: dolines.
inline float sinkSolution(float solubility, float sunk, float cell) { return solubility * sunk / (cell * cell); }

} // namespace derivean::sim
