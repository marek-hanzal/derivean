#pragma once

namespace derivean::sim {

// The era counts years since the hot initial body. Every level steps on a grid anchored at
// absolute multiples of its step, so jumping to a date and running up to it visit the same states.
inline constexpr double Myr = 1e6;
inline constexpr double Gyr = 1e9;

// Early cooling is solved in closed form; later processes may only start after these marks.
inline constexpr double MagmaOceanEnd = 3 * Myr;       // first lasting quench crust
inline constexpr double CrustStable = 30 * Myr;        // resurfacing by lava has become local
inline constexpr double RainStart = 40 * Myr;          // surface drops below the steam condensation point
inline constexpr double OceansFull = 150 * Myr;        // the surface water inventory has rained out
inline constexpr double TectonicsStart = 200 * Myr;    // rigid plates start to move

inline constexpr double PlanetStep = 2 * Myr;          // L0, whole planet
inline constexpr double RegionStep = 400e3;            // L1, 1 km cells
inline constexpr double LocalStep = 100e3;             // L2, 64 m cells
inline constexpr double SiteStep = 20e3;               // L3, 4 m cells

inline constexpr double GlacialCycle = 100e3;          // years between glacial terminations

} // namespace derivean::sim
