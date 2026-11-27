#pragma once

#include "aeolian/type/Regime.hpp"

#include <cstdint>

namespace derivean::sim {

// m/s of friction velocity at which dry grains of a size start to hop (Bagnold).
double thresholdFriction(double grain);

// m²/yr of loose sand per metre of width that a steady wind of `speed` m/s at WindHeight carries
// over a dry, flat, sand-covered bed (Lettau and Lettau); zero below the threshold.
double saturatedFlux(double speed);

// The sand rose of a monthly mean wind with gusts and passing weather of `spread` m/s around it, added
// into `regime.drift` with the weight of that month.
void addMonth(WindRegime& regime, Vec2 monthly, double spread, double weight);

// Totals of a filled rose.
void closeRose(WindRegime& regime);

// The regime at a latitude in degrees, from what the climate knows about wind in closed form:
// the belts (beltWind), their shift with the sun through the year, the unresolved gustiness and the
// passing weather systems of the storm track (storminess). `northZ` is northwardZ at the place and
// `gradient` the fall of the annual mean temperature in °C per degree of latitude.
// Absent, because the climate does not hold them: winds steered or funnelled by relief, sea and
// slope breezes, and the reversal of a monsoon.
WindRegime regimeAtLatitude(double latitude, double northZ, double gradient);

// The annual mean wind of regimeAtLatitude alone, the same to the bit, without the rose.
Vec2 meanWindAtLatitude(double latitude, double northZ);

// The regime at a world position as the landscape reads the planet there.
WindRegime regimeAt(uint64_t seed, Vec2 position, double time);
// The same at the planet point that position reads (planetPoint).
WindRegime regimeAtPoint(uint64_t seed, Vec2 point, double time);

// A regime of steady winds: `speeds[k]` m/s blowing towards `headings[k]` radians (from +X towards
// +Z) for `shares[k]` of the year. For hand-built beds.
WindRegime regimeOf(int winds, const double* headings, const double* speeds, const double* shares);

// Share of the year a bare loose surface is dry enough to blow, from rain (m/yr) and the annual
// mean temperature at the ground (°C); ground that is both frozen and wet does not blow at all.
double dryShare(double rain, double warmth, double halfRange);
// 0..1: the share of what the air could take from the ground that the rain leaves it; 0 where rain
// keeps up. How seldom running water and plants hold a place, as against how often its surface is dry.
float aridityOf(float rain, float celsius);
// Share of the year a stream fed by the rain of its own ground runs: all of it where the rain keeps
// up with the air, the few flood days of the driest country (`FloodShare`) where none does.
float runningShareOf(float rain, float celsius);

} // namespace derivean::sim
