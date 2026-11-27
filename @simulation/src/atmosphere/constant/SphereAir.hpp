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

} // namespace derivean::sim
