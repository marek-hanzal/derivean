#pragma once

#include "atmosphere/type/LatitudeBands.hpp"
#include "atmosphere/type/SphereSeason.hpp"

#include <vector>

namespace derivean::sim {

// The season on day `day` of the year (the flat calendar's declinationOn) from the zonal air
// temperature per band, °C, and the same of the annual-mean climate, `base`. As on the flat world:
// - the belts (beltWind) follow the sun BeltShift degrees per degree of declination, BeltLag days
//   late, and blow JetDrive times as hard as each half's subtropics (15° to 35°) are warmer than its
//   subpolar bands (55° to 75°), against the same contrast of `base`, clamped to 0.5 to 1.8; each
//   contrast is a difference of area-weighted means (B_b), so a constant cancels, as between the
//   flat world's equal counts of rows;
// - the jet is that zonal wind's vorticity by Stokes (zonalWindVorticity), so Σ_b B_b jet_b telescopes
//   to zero (balancedJet);
// - the cells' surface branch is the belts' poleward wind, its convergence through the boundaries'
//   circles lifting MoistDepth · LiftTime of air per unit, so Σ_b B_b overturning_b is zero too;
// - growth is EddyGrowth · storminess of the band's temperature gradient (°C per degree, centred; a
//   polar band takes itself for the band past the pole, as the flat world's rows wrap) at its
//   latitude, the two-layer supercriticality (Phillips 1954; Stone 1978), and the shear the thermal
//   wind of that gradient (Holton & Hakim 2012), westerly where the air cools poleward,
//   ShearPerGradient · sin 2|φ| per °C per degree, at most ShearLimit either way.
// Each half needs a band centred in [15°, 35°) and one in [55°, 75°), as every count from 8 has (the
// grid's side, the count to use, is 8 at level 3), and the base a positive contrast in each; both
// fields one finite value per band and a finite day; std::invalid_argument otherwise. The insolation is S4.4's, from
// S2.
SphereSeason sphereSeasonOf(const LatitudeBands& bands, double day, const std::vector<double>& temperature,
                            const std::vector<double>& base);

// Per band, a profile given at two or more finite `latitude`s (degrees, strictly monotone either way)
// with a finite value each, std::invalid_argument otherwise, read at the band's centre, linear between
// the two points around it and held at the ends: one pole-to-pole half of a flat world's rows (whose
// latitude runs 90 → −90 → 90), or zonalClimate's, as the bands' prescribed temperature (S4 plan, the
// runs of S4.3).
std::vector<double> bandProfile(const LatitudeBands& bands, const std::vector<double>& latitude, const std::vector<double>& value);

} // namespace derivean::sim
