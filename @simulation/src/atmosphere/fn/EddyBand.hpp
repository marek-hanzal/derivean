#pragma once

#include "atmosphere/type/EddyBand.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// The band for a grid, centred on a wavelength (m) with a half-width given as a share of the centre
// wavenumber. SphereEddyWavelength and SphereEddyReach (constant/SphereAir.hpp) are the flat world's
// band at the same metres.
EddyBand makeEddyBand(const SphereGrid& grid, double wavelength, double reachShare);

double eddyGain(const EddyBand& band, int degree);

// band = Σ over degrees and orders m ≠ 0 of gain(l) · (the field's coefficient) · Y_lm. The
// coefficients come from the area-weighted sum over the cells (the grid's own quadrature), so it is
// second order: a harmonic of unit mean square comes back within 2.2e-3 at level 6 and a mode outside
// the band leaks as much, both falling fourfold per level. The harmonics are evaluated by recursion
// in every call (no tables: 6 × 256² cells × 300 modes would be 0.9 GB).
void filterEddyBand(EddyBand& band, const std::vector<double>& field, std::vector<double>& result);

} // namespace derivean::sim
