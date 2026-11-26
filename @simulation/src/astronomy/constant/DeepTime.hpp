#pragma once

namespace derivean::sim {

// The planet's age at which the sky is today's: the world's default era (`World::age`). Earth formed
// 4.54 Gyr ago and the Sun 4.57 Gyr ago; the 0.03–0.06 Gyr between them and this round value move
// the luminosity by under 1 % and the Moon by under 0.01 %.
inline constexpr double PresentAge = 4.6e9;                 // yr since the hot initial body

// The Moon's distance before today follows the tidal rhythmites (Williams 2000, Rev. Geophys. 38:37):
// a/a₀ = 0.965 ± 0.005 at 620 Ma (Elatina), then a mean recession of 1.24 ± 0.71 cm/yr from 2450 to
// 620 Ma (Weeli Wolli to Elatina). After today the Moon recedes from the lunar-laser-ranging rate of
// 3.82 cm/yr (Dickey et al. 1994) under a constant tidal Q (ȧ ∝ a^-11/2; Murray & Dermott 1999), an
// extrapolation.
inline constexpr double ElatinaAge = 620e6;                 // yr before today
inline constexpr double ElatinaMoonShare = 0.965;           // a/a₀
inline constexpr double ProterozoicRecession = 0.0124;      // m/yr
inline constexpr double OldestRhythmite = 2450e6;           // yr before today
inline constexpr double MoonRecessionToday = 0.0382;        // m/yr

// The Sun brightens on the main sequence (Gough 1981, Solar Phys. 74:21): L = L☉ / (1 + 0.4 (1 − t/t☉)),
// a fit to standard solar models good to a few per cent from the zero-age main sequence to today.
inline constexpr double SunAgeToday = 4.57e9;               // yr, t☉
inline constexpr double GoughSlope = 0.4;

} // namespace derivean::sim
