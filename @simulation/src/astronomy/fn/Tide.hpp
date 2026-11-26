#pragma once

#include "astronomy/type/Sky.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// The equilibrium tide of the Moon and the Sun at their real positions (degrees 2 and 3 of the
// tide-generating potential, Cartwright & Tayler 1971). The ocean's dynamic response is later work.

// m²/s² at an observer (Earth frame, m); its mean over a sphere is zero, so the tide only moves water.
double tidePotential(const Sky& sky, Vec3 observer);

// m: the potential over gravity, the sea surface's equilibrium on a rigid planet (the Moon alone
// raises 0.36 m under it at its mean distance). Against the yielding sea floor the observed equilibrium
// tide is (1 + k₂ − h₂) = 0.69 of this (Love numbers of the IERS Conventions, Petit & Luzum 2010).
double equilibriumTide(const Sky& sky, Vec3 observer);

} // namespace derivean::sim
