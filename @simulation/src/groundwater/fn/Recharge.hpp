#pragma once

#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// What a patch of ground offers the rain.
struct Soak {
    float rain{};                      // m/yr
    float warmth{};                    // °C, annual mean at the ground
    float slope{};                     // rise over run of the ground
    float cover[SedimentClasses]{};    // m of coarse, sand and fine cover as they lie on the rock
    float ice{};                       // m of glacier
    Material rock{Material::Bedrock};  // bed at the top of the solid rock
};

// 0..1: the share of the water evaporation leaves (`runoffOf`) that enters the ground instead of
// running off over it. It is a closed form of the cell alone, so the landscape can take the same
// share out of the runoff it routes that the ground water model takes in:
//   - rain falls in storms; what the topmost layer cannot take in at storm intensity runs off;
//   - a thin cover on tight rock fills in one storm and sheds the rest;
//   - water runs off steep ground before it soaks in;
//   - frozen ground and glacier ice let nothing through;
//   - over the year no more gets in than leaves the cover, down into the rock or along the cover,
//     and no more goes into the rock than its opened zone and joints carry to the slope's outlet.
float infiltrationShare(const Soak& soak);

} // namespace derivean::sim
