#pragma once

#include "chronology/type/Calendar.hpp"

namespace derivean::sim {

// What lies on and over the ground, as the heat budget, the water budget and the weather samples
// all read it.
inline constexpr double SnowCoverMass = 20.0;   // kg/m² of snow that hides the ground completely
inline constexpr double LandDaily = 5.0;        // °C half-range between afternoon and the end of the night over bare land under a high clear sun
inline constexpr double SeaDaily = 0.5;         // °C, the same over open water
inline constexpr double CloudDamping = 0.6;     // share of that range a fully shading sky removes

// −1 at the end of the night to 1 in mid-afternoon, scaled by how high the sun climbs that day.
inline double dailySwing(double latitude, double declination, double hour) {
    constexpr double Pi = 3.141592653589793;
    const double noon = std::max(0.0, std::sin((90.0 - std::abs(latitude - declination)) * Pi / 180.0));
    return noon * std::cos(2 * Pi * (hour - 15.0) / 24.0);
}

} // namespace derivean::sim
