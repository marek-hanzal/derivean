#pragma once

namespace derivean::sim {

// Today's Sun, Earth and Moon. Earth is the reference: values are measured ones, rounded only where
// the model is coarser than the rounding. Sources (docs/handoff/astronomy.md, Sources): the IAU 2015
// nominal solar values (Prša et al. 2016), the IAU 2012 astronomical unit, CODATA 2018 (Tiesinga et al.
// 2021), the IERS Conventions 2010 (Petit & Luzum 2010), DE440 (Park et al. 2021), Yoder 1995 and
// Meeus 1998 ch. 12, 25, 31 and 47. Angles are degrees, times SI seconds.

inline constexpr double JulianDay = 86400.0;                      // s
inline constexpr double JulianYear = 365.25 * JulianDay;          // s
inline constexpr double J2000 = 2451545.0;                        // Julian date (TT) of 2000-01-01 12:00 TT

// Bodies.
inline constexpr double GravitationalConstant = 6.6743e-11;       // m³ kg⁻¹ s⁻² (CODATA 2018)
inline constexpr double SunGM = 1.32712440018e20;                 // m³/s²
inline constexpr double EarthGM = 3.986004418e14;                 // m³/s²
inline constexpr double MoonGM = 4.9028000661e12;                 // m³/s²
inline constexpr double SunRadius = 6.957e8;                      // m, photosphere
inline constexpr double MoonRadius = 1.7374e6;                    // m
inline constexpr double SolarLuminosity = 3.828e26;               // W today: 1361 W/m² at 1 AU
inline constexpr double EarthMomentOfInertia = 8.034e37;          // kg m², about the spin axis (0.3307 M R²)
inline constexpr double AstronomicalUnit = 1.495978707e11;        // m

// Earth's orbit (of the Earth–Moon barycentre), mean elements at J2000 (Meeus 1998, 25.2–25.4 and
// table 31.A); the years follow from their rates and the IAU 1976 precession (Lieske et al. 1977), 5029"
// per century. The three years differ by
// what they count: the seasons repeat with the tropical year, the orbit closes against the stars in the
// sidereal year and perihelion returns after the anomalistic year. The spin axis precesses about the
// ecliptic pole by the difference between the first two (one turn in 25 772 yr); the orbit's perihelion
// advances by the difference between the last two.
inline constexpr double EarthOrbitAxis = 1.000001018 * AstronomicalUnit;   // m
inline constexpr double EarthOrbitEccentricity = 0.016708634;
inline constexpr double TropicalYear = 365.24219 * JulianDay;
inline constexpr double SiderealYear = 365.256363 * JulianDay;
inline constexpr double AnomalisticYear = 365.259636 * JulianDay;
inline constexpr double SunMeanAnomalyJ2000 = 357.52911;
inline constexpr double SunPerigeeJ2000 = 282.93735;               // the Sun's geocentric perigee: perihelion + 180°
inline constexpr double Obliquity = 23.4392911;                    // held; its 41 kyr cycle belongs to S4b

// Earth's rotation. The sidereal day is one turn against the equinox of date; Greenwich's mean
// sidereal angle at J2000 UT1 anchors the phase (Meeus 12.4). TT − UT1 is held at its value of
// 2024–2026; its real drift (about +1 s per year lately) is the measured tidal braking and core
// coupling, which the deep-time model covers on its own scale.
inline constexpr double SiderealDay = 86164.0905;
inline constexpr double GreenwichSiderealJ2000 = 280.46061837;
inline constexpr double DeltaT = 69.2;                             // s, TT − UT1

// The Moon's orbit about the Earth, mean elements at J2000 (Meeus 47.1–47.7). The node regresses
// and the perigee advances against the equinox in the given periods (18.61 and 8.85 years); the
// synodic month of 29.53 days follows from the sidereal month and the sidereal year.
inline constexpr double MoonOrbitAxis = 3.844e8;                   // m
inline constexpr double MoonEccentricity = 0.0549;
inline constexpr double MoonInclination = 5.145;                   // to the ecliptic
inline constexpr double SiderealMonth = 27.321661 * JulianDay;
inline constexpr double NodalPeriod = 6798.38 * JulianDay;
inline constexpr double ApsidalPeriod = 3231.50 * JulianDay;
inline constexpr double MoonMeanLongitudeJ2000 = 218.3164477;
inline constexpr double MoonMeanAnomalyJ2000 = 134.9633964;
inline constexpr double MoonArgumentOfLatitudeJ2000 = 93.2720950;

} // namespace derivean::sim
