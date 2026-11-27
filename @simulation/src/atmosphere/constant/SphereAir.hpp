#pragma once

#include "planet/constant/Scale.hpp"

namespace derivean::sim {

// The eddy band of the flat world (EddyWave ± EddyBand = 7 ± 3.5 cycles across PlanetSpan in
// atmosphere/fn/Circulation.cpp, which owns them) at the same metres on the sphere: weather systems of
// 2 397 km grow fastest, the band reaching from 4 793 to 1 598 km, which on Earth's radius are the
// spherical-harmonic degrees 8 to 24. This keeps the closure while the geometry changes; it is not yet
// Earth's band. Earth's baroclinic waves grow fastest near 4 000 km (degree 10, zonal wavenumber 7 at
// 45°), while today's metres give zonal wavenumber 12 there; keeping the flat world's 7 cycles round
// the planet instead would give 5 700 km. Which one the switch takes is S4's decision
// (docs/handoff/atmosphere-sphere.md, eddy band); makeEddyBand takes any wavelength.
inline constexpr double SphereEddyWavelength = PlanetSpan / 7.0;   // m
inline constexpr double SphereEddyReach = 0.5;                     // half-width, share of the centre wavenumber

// The flat world's closures at the same seconds and metres (atmosphere/fn/Circulation.cpp owns them
// until the switch): the zonal mean returns to its belts over MeanRelaxation, friction is a rate of
// (1 + h / RoughHeight) / SurfaceDrag at terrain height h, and the damping's fastest mode decays
// over GridDamping.
inline constexpr double SphereMeanRelaxation = 4 * 86400.0;   // s
inline constexpr double SphereSurfaceDrag = 12 * 86400.0;     // s
inline constexpr double SphereRoughHeight = 1500.0;           // m
inline constexpr double SphereGridDamping = 6 * 3600.0;       // s

// The flat world's season at the same degrees and seconds (atmosphere/fn/Season.cpp owns them until
// the switch): the belts follow BeltShift degrees per degree of solar declination BeltLag days late,
// blow JetDrive times their climatological speed scaled by each half's temperature contrast, feed
// weather systems at up to EddyGrowth, and the westerlies shear by ShearPerGradient per °C per degree
// of latitude, at most ShearLimit.
inline constexpr double SphereBeltShift = 0.35;                       // degrees per degree of declination
inline constexpr double SphereBeltLag = 30.0;                         // days
inline constexpr double SphereJetDrive = 1.6;
inline constexpr double SphereEddyGrowth = 1.0 / (1.5 * 86400.0);     // 1/s
inline constexpr double SphereShearPerGradient = 12.0;                // m/s per °C per degree
inline constexpr double SphereShearLimit = 12.0;                      // m/s

// From PolarCap degrees to a pole the belts' zonal wind tapers to zero (sphereSeasonOf). A wind
// tangent to the sphere has a value at a pole only if its zonal mean is zero there, and Stokes round
// a cap of angular radius θ gives it the vorticity u / (R tan(θ/2)), which grows as 1/θ as the grid
// refines while u does not vanish; the belts' wind, JetDrive times the flat world's beltWind (whose
// rows wrap and have no pole), keeps −1.18 m/s there (Codex DREV-CLAUDE-010-01). 75° is a convention,
// the comparison probe's of DREV-CLAUDE-010-01: the middle of the annual-mean belts' polar cell (60° to
// 90°, the zeros of beltWind's poleward branch; the season moves its equatorward edge, not the cap),
// so the easterlies keep their speed over the cell's equatorward half and weaken over its poleward
// half. Its cosine is a literal, so no libm call sets the taper (docs/windows.md#determinism).
inline constexpr double SpherePolarCap = 75.0;                          // degrees
inline constexpr double SpherePolarCapCosine = 0.25881904510252074;     // cos 75°

} // namespace derivean::sim
