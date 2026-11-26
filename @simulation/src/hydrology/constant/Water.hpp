#pragma once

#include "chronology/constant/Timeline.hpp"

namespace derivean::sim {

inline constexpr double Gravity = 9.81;              // m/s²
inline constexpr double OpenWater = 1e-3;            // m of depth from which a cell is open water rather than wet ground
inline constexpr double BedRoughness = 0.035;        // Manning coefficient of a natural bed, s/m^(1/3)
inline constexpr double OrificeCoefficient = 0.6;    // discharge coefficient of a sharp-edged opening

// Composition. Running fresh water carries a little of the rock it has dissolved; the sea is what
// was left behind when that water evaporated, so its salinity grows towards a mature value with
// the age of the oceans. Both are stated initial conditions, not results.
inline constexpr double RiverSalinity = 0.12;        // kg/m³
inline constexpr double MatureSalinity = 35.0;       // kg/m³
inline constexpr double SalinityTime = 800 * Myr;    // e-folding of the approach to it, counted from RainStart

// m/yr the air can take from open water at an annual mean temperature in °C. The landscape model
// closes its lakes with the same expression; the two must stay equal for an untouched lake to be
// at rest when it becomes live.
inline constexpr float evaporationDemand(float celsius) {
    const float demand = 0.25f + 0.035f * celsius;
    return demand < 0.05f ? 0.05f : demand > 1.6f ? 1.6f : demand;
}

// Sediment of the live surface.
inline constexpr float DeltaFreeboard = 0.02f;       // m of water a delta leaves above itself
inline constexpr int DeltaReach = 4096;              // cells one delta lobe may spread over in a step
inline constexpr double CutShare = 0.5;              // most of its drop to the next cell a bed may lose in a step

} // namespace derivean::sim
