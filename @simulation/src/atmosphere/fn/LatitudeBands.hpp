#pragma once

#include "atmosphere/type/LatitudeBands.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// `count` bands (at least 2) over the grid. A cell's latitude is asin of its centre's z, once here.
LatitudeBands makeLatitudeBands(const SphereGrid& grid, int count);

// Per band, the area-weighted mean of a cell field: Σ_i A_i w_ib f_i / B_b.
void bandMeans(const LatitudeBands& bands, const SphereGrid& grid, const std::vector<double>& field, std::vector<double>& mean);

// Per cell, Σ_b w_ib g_b: a band field back on the cells. Σ_i A_i (spread)_i = Σ_b B_b g_b.
void spreadBands(const LatitudeBands& bands, const std::vector<double>& perBand, std::vector<double>& field);

// Per band, the mean vorticity of a zonal wind by Stokes: `eastward` holds u (m/s) at the count + 1
// boundaries (LatitudeBands::boundary), and band b's vorticity is (rim_b u_b − rim_{b+1} u_{b+1}) / B_b, the circulation round
// its southern boundary minus that round its northern one (both counted eastward, the band on the
// left of the southern one). Over all bands Σ_b B_b ζ_b telescopes to zero, so a vorticity built so
// and spread to the cells has zero area mean to rounding, as a vorticity of the whole sphere must.
void zonalWindVorticity(const LatitudeBands& bands, const std::vector<double>& eastward, std::vector<double>& vorticity);

} // namespace derivean::sim
