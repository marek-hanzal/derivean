#pragma once

namespace derivean::sim {

// One moisture budget serves the stepped atmosphere and the steady annual-mean climate. Both read
// these constants and the laws in climate/fn/Moisture.hpp, so the two can only differ in how they
// treat time. Water is counted in kg; a kg/m² of water is one millimetre.
inline constexpr double WaterDensity = 1000.0;           // kg/m³

inline constexpr double VapourDepth = 2200.0;            // m; a saturated column holds this depth of surface-saturated air
inline constexpr double ExchangeCoefficient = 1.3e-3;    // bulk transfer between a wet surface and the air
inline constexpr double Gustiness = 3.0;                 // m/s of unresolved wind that ventilates a calm surface

// Condensation begins before the whole column is saturated, because part of a cell always is.
inline constexpr double CondensationOnset = 0.85;        // column vapour against capacity
inline constexpr double CondensationTime = 3 * 3600.0;   // s to condense one capacity at saturation
inline constexpr double LiftDepth = 2500.0;              // m of lifting that halves what the column can hold
inline constexpr double LiftTime = 12 * 3600.0;          // s a column spends in the ascent that acts on it; turns m/s into m of lift
inline constexpr double MoistDepth = 4000.0;             // m of air the overturning cells converge or spread

// Low air does not climb a high barrier, it is held back or flows around: however tall the
// terrain, the moist layer is lifted by no more than this.
inline constexpr double BlockedLift = 400.0;             // m

inline constexpr double SoilCapacity = 80.0;             // kg/m² bare ground holds within reach of the air before it runs off
inline constexpr double SnowLine = 1.0;                  // °C at the ground below which precipitation is snow
inline constexpr double SnowBlend = 1.0;                 // °C on either side of the snow line where both fall
inline constexpr double DegreeDayMelt = 4.0;             // kg/m² of snow melted per day and °C above freezing

} // namespace derivean::sim
