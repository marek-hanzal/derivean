#pragma once

#include "astronomy/type/Sky.hpp"
#include "geometry/type/Vec3.hpp"

#include <span>

namespace derivean::sim {

// The equilibrium tide of the Moon and the Sun at their real positions (degrees 2 and 3 of the
// tide-generating potential, Cartwright & Tayler 1971). The tide is a force that moves water; it
// makes none. Its potential averages to zero over the whole sphere only. Over an ocean that covers
// part of it the mean is not zero: with both bodies overhead at their mean distances it is +0.20 m
// over the cap z ≥ 0.5. So a sea never adds `equilibriumTide` to its level. It takes `wetTide` for a
// fixed mask, or drives its own flow with the potential. The ocean's dynamic response is later work.

// m²/s² at an observer (Earth frame, m).
double tidePotential(const Sky& sky, Vec3 observer);

// m: the potential over gravity, the planet-wide shape of the equilibrium surface on a rigid planet
// (the Moon alone raises 0.357 m under it at its mean distance). Against the yielding sea floor, the
// observed equilibrium tide is (1 + k₂ − h₂) = 0.69 of this (Love numbers of the IERS Conventions,
// Petit & Luzum 2010). Not a height to add: see above.
double equilibriumTide(const Sky& sky, Vec3 observer);

// m per wet cell: the equilibrium tide over a fixed wet mask. The caller passes the wet cells' positions
// (Earth frame, m) and true areas (m², geometry's), and receives each cell's height.
// - The planet-wide shape is shifted by its area-weighted mean over those cells, so Σ height × area is
//   zero and the water's volume does not change.
// - If the mask changes, call it again with the new mask. It conserves only the tidal displacement over
//   that mask; the water that newly wet or dried cells gain or lose is the sea owner's volume solve.
// - Where the coast follows the bathymetry instead of a fixed mask, the common level correction comes
//   from the volume over the real bathymetry. That belongs to the sea's owner, not here.
// - The sizes must match, or it throws std::invalid_argument.
void wetTide(const Sky& sky, std::span<const Vec3> wet, std::span<const double> areas, std::span<double> heights);

} // namespace derivean::sim
