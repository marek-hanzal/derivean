# Astronomy hand-over

Owner of this file: the astronomy stream, slice S2 of the planet (`@simulation/src/astronomy/**`, `@simulation/test/Astronomy*.{cpp,hpp}`). It describes the present.

## 1. Goal

The owner's order of 26 Nov 2026: the Sun and the Moon are real bodies at real positions, and the planet moves in space, so "winter is colder, not as a switch, but from the physical position in space relative to the Sun". Earth is the reference. Plan: `.out/lead/handover/PLANET.md`, decisions 5 and 6 and the S0 review (Q8 has precedence).

## 2. State now

Builds; `simulation-tests astronomy` passes (about 0.3 s). The domain is additive: nothing calls it yet, and `chronology/**` (`Calendar.hpp`'s `hourOfDay`, `declinationOn`, `lunarPhase`, `tideHeight`) is unchanged. S4 and S6 move the consumers.

It depends on geometry (S1) for one type, `geometry/type/Vec3.hpp` (ECEF), and the tests use `geometry/constant/Sphere.hpp` (`PlanetRadius`).

## 3. What exists

One time input, one snapshot, free functions on the snapshot.

- `type/Instant.hpp`: `Instant{epoch, seconds, age}`. `epoch` is the Julian date (TT) of second zero, `seconds` the SI seconds since then, `age` the planet's age in years (as `Calendar::era`). The epoch places second zero on today's ephemeris; from it, time runs at the rates of the age.
- `fn/Sky.hpp`: `skyAt(Instant) → Sky` (`type/Sky.hpp`), about 0.6 µs. A `Sky` holds the geocentric Sun and Moon positions in metres in the Earth frame (`sun`, `moon`) and in the inertial frame (`sunInertial`, `moonInertial`), the sidereal angle, the precession, the mean Sun's longitude, and the deep-time state.
- `fn/Orbit.*`: mean elements and their rates. The Sun's orbit is a Kepler ellipse of the Earth–Moon barycentre with the Earth's swing about it. The Moon's is a Kepler ellipse (a, e, i) with its node regressing and its perigee advancing, plus the Sun's perturbations of 0.01° and more (Meeus tables 47.A and 47.B: evection, variation, annual equation and their kin).
- `fn/Frame.*`: the J2000 ecliptic → precession → equator of date → Earth frame by the sidereal angle; inertial ↔ ecliptic.
- `fn/Place.*`: what a place sees. The caller gives `up` (unit, Earth frame) where only the direction matters, or `observer` (Earth-frame position, m) where the distance matters (the Moon's parallax is 0.95° on average and up to 1.02° at perigee). Functions: `sunDirection`, `moonDirection`, `sunElevation`, `moonElevation`, `sunDeclination`, `sunAngularRadius`, `moonAngularRadius`, `solarTime` (apparent), `meanSolarTime`, `equationOfTime`, `irradiance` (1/r², luminosity of the age), `insolation` (on level ground above the air).
- `fn/Eclipse.*`: `sunCoverage(sky, observer)` (share of the solar disc hidden), `solarShadow(sky)` (the Moon's shadow on the fundamental plane: axis distance, penumbra and umbra radii, north or south), `lunarShadow(sky, planetRadius)` (penumbral and umbral magnitudes, the share of the disc in the umbra).
- `fn/Moonlight.*`: `lunarAge` (0 new, 0.5 full), `moonIllumination(sky, observer)`, `moonSunward` (from the Moon to the Sun).
- `fn/Tide.*`: `tidePotential` (degrees 2 and 3, Moon and Sun), `equilibriumTide` (potential over gravity: the planet-wide shape on a rigid planet, not a height to add to a sea) and `wetTide` (that shape over a fixed wet mask, with the mask's own level correction, so the water's volume stays the same).
- `fn/DeepTime.*`: `deepTimeAt(age) → DeepTime` (`type/DeepTime.hpp`: Moon's axis, sidereal month, nodal and apsidal periods, sidereal day, luminosity, `calibrated`) and `solarDay(siderealDay)`.
- Constants: `constant/Sky.hpp` (bodies, orbits, rotation; measured values with sources), `constant/DeepTime.hpp` (the record's anchors).

## 4. Decisions

- **Frames.**
  - The Earth frame is geometry's ECEF: Z to the north pole, X to 0° N 0° E, Y to 0° N 90° E.
  - The inertial frame is the mean equator and equinox of J2000 (EME2000, within 0.02" of ICRF), so Horizons' ICRF vectors compare directly.
  - Up, north, east, latitude and longitude belong to S1. Astronomy takes a place as `up` or `observer` and never as latitude and longitude. Azimuth is not computed here; it follows from the vectors and S1's frame.
- **Three years.**
  - The seasons repeat with the tropical year (365.24219 d).
  - The orbit closes against the stars in the sidereal year (365.256363 d).
  - Perihelion returns in the anomalistic year (365.259636 d).
  - The axis precesses about the ecliptic pole by the gap between the tropical and the sidereal year (25 772 yr). So the tropical year is not a period of the orbital elements; the test finds ten years of March equinoxes 365.2422 d apart on average.
  - **This overrides PLANET.md decision 5** ("spin axis fixed in inertial space"). Q8 asks for a tropical year apart from the elements' periods, and a fixed axis would make the seasons repeat with the sidereal year. Within a session the axis is practically fixed: 0.014° a year.
- **The axis.** The obliquity is held at 23.4392911°. The ecliptic is held (its drift is 47" per century). Nutation is left out (±17").
- **Rotation.** The sidereal day is 86 164.0905 s against the equinox. The phase is Greenwich's mean sidereal angle at the epoch (Meeus 12.4), with UT1 = TT − 69.2 s (ΔT of 2024–2026, held).
- **The Sun's position is geometric.** Light time and aberration are left out (20.5"). The Earth's swing about the barycentre is in (up to 6").
- **Moon perturbations.** Terms of M′ and F alone are solved exactly by the ellipse and the inclined plane. The exception is sin(M′ − 2F): the plane gives −0.012666° of Meeus's +0.010980°, so the series carries the Sun's 0.023646°. The rest come from the series. Their amplitudes are today's at every age.
- **Refraction belongs to the air.** Elevations are geometric. The tests use the −0.8333° sunrise convention (34′ refraction plus 16′ semidiameter) only to compare with references.
- **Deep time.**
  - The planet's age 4.6 Gyr (the world's default era, `World::age`) is today's sky.
  - The Moon's distance before today follows the rhythmites (Williams 2000): 0.965 a₀ at 620 Ma, then 1.24 cm/yr back to 2450 Ma, extrapolated further.
  - After today the Moon recedes from 3.82 cm/yr under a constant tidal Q (a^13/2 linear in time).
  - The day follows from conserving the Earth–Moon angular momentum (C = 8.034 × 10³⁷ kg m²), with no fitted factor. The spin cannot fall below the month.
  - Sidereal month ∝ a^3/2; nodal and apsidal periods ∝ a^−3/2 (the Sun drives both precessions at n☉²/n☾).
  - Luminosity: Gough (1981) on the Sun's age, held beyond 2 t☉.
  - Rates are held over a session at the age of second zero: the day lengthens 2 ms per century, far below what a session sees.
  - Held at today's at every age: the axis's precession rate (it depends on the spin and the Moon's distance), the obliquity and both eccentricities.
  - The spin would lock to the month only near 20 Gyr, beyond the world's 12 Gyr.
- **Not here:**
  - Milankovitch cycles: La2010 is an exact ephemeris only to about 50 Myr, so S4b treats them as cycles.
  - Love numbers and the ocean's response: `equilibriumTide` is the rigid planet; the observed equilibrium tide is 0.69 of it.
  - Limb darkening in eclipse coverage.
  - Planets and stars.

## 5. Range and accuracy

Against JPL Horizons (DE441), six dates 1987–2026 and four sites at 0°, 139.8° E, 99.1° W and 18.96° E; the queries are kept in `.out/astronomy/horizons/*.sh`. Largest errors over the set, followed by the test's tolerance:

What each reference is:
- Horizons' RA and Dec are astrometric: they include the light time, but not aberration or nutation.
- Its elevations are "apparent airless". They include aberration, nutation and the site's own corrections, but no refraction.
- The model is a geometric one on a spherical Earth, so these are useful comparisons within the stated tolerances, not identities.

The numbers:

- **Sun**
  - direction: 0.0051° (tolerance 0.02°)
  - distance: 4.2 × 10⁻⁵ (tolerance 10⁻⁴)
  - topocentric elevation: 0.061° (tolerance 0.08°). Most of it is the held ΔT in 1987: 13.6 s of turn.
- **Moon**
  - direction: 0.041° (tolerance 0.1°); the reviewer's check over Horizons' hourly 2015 and 2025 gives 0.046° at most and 0.017° rms
  - distance: 8.5 × 10⁻⁴ (tolerance 2 × 10⁻³)
  - topocentric elevation: 0.042° (tolerance 0.15°)
- **Sunrise** (Prague in summer and winter, the equator at an equinox, Tokyo): within 1 min of Horizons interpolated to −0.8333°.
- **Year**
  - The 2024 and 2025 March equinoxes of the geometric Sun come 9.3 and 9.5 min before the published ones less the 8.3 min of aberration (tolerance 10 min).
  - The 2024–2025 interval is 365.2464 d (real 365.2465 d, tolerance 3 min). Ten years give the mean tropical year to 5 × 10⁻⁴ d.
  - Perihelion and aphelion of 2024 fall within 0.5 h and 1.2 h, and 1.8 × 10⁻⁵ AU and 1.5 × 10⁻⁵ AU, of Horizons (tolerances 6 h and 5 × 10⁻⁵ AU).
  - Irradiance is 1361 W/m² at 1 AU, +3.4 % at perihelion and −3.3 % at aphelion.
- **Equation of time:** 822.1 s against Meeus' 822.6 s (example 28.b). The yearly extremes are −14.2 min and +16.4 min within 0.2 min and 3 days.
- **Eclipses of 2024–2025**, from the geometry alone (NASA's canon):
  - All eight are found, with nothing extra. Greatest phases fall within 2.0 min (solar) and 4.9 min (lunar) of the canon's TD times (tolerance 10 min).
  - Solar eclipses are of the right kind (total, annular, partial). γ is within 0.028 of NASA's (0.369, −0.347, 1.068, −1.060 against 0.343, −0.351, 1.041, −1.065).
  - Lunar umbral magnitudes are within 0.04 (−0.124, 0.049, 1.211, 1.338 against −0.130, 0.087, 1.180, 1.362).
  - Under the shadow's axis, the local Sun is hidden wholly at the total eclipse and to a ring at the annular one. This check, and the eclipses falling at new and full moon, use the same `Sky`, so they test consistency between the functions. Only the catalogue values are independent.
- **Moon**
  - The synodic month comes out 29.5306 d (within 0.02 d over 24 lunations).
  - The yearly reach of the declination is within 0.1° of Horizons: 28.72° in 2025 (major standstill) and 18.65° in 2015.
- **Tide**
  - The mean over the whole sphere is zero.
  - Over the cap z ≥ 0.5, with both bodies overhead at their mean distances, the planet-wide shape averages +0.1956 m (Codex measured +0.1955 m). `wetTide` brings that cap, and two uneven masks at two instants (the second larger), to a volume of zero (relative 10⁻¹²) without changing the shape. Water carried between masks by wetting and drying is the sea owner's solve and is not tested here.
  - Spring and neap ranges are within 3 % of 1.5 (A☾ + A☉) and 1.5 A☾.
  - The Moon raises 0.357 m at its mean distance.
- **Deep time**
  - The Moon's distances at 620 Ma (0.965 a₀) and 2450 Ma (0.906 a₀) are the model's input calibration. The tests check that the input holds; they are not an independent estimate.
  - At 620 Ma: a day of 22.09 h (Williams: 21.9 ± 0.4 h), 396.7 days a year (400 ± 7) and 13.10 synodic months (13.1 ± 0.1). The 22.09 h is what the model computes from the masses, the moment of inertia and the Earth–Moon angular-momentum balance it uses. It agrees with the record; it was not fitted to it.
  - At 2450 Ma: a day of 19.43 h.
  - At age 0 (the hot body): a 16.9 h day, 0.837 a₀ and 0.714 L☉. This is an extrapolation and the real early Moon was much closer.
  - At 12 Gyr: an 81.5 h day, a 41.0 d month, 1.31 a₀ and 1.667 L☉ (held). The real Sun leaves the main sequence before then.
  - `calibrated` is true from 2450 Ma to today.

Present-day validity:
- The range checked against Earth is 1987–2026. The ephemeris is linear in time from J2000. Outside that range the errors grow slowly, mostly from the held ΔT (about 1 s per year) and the held obliquity (0.013° per century).
- Over millennia, use it as a model of Earth's sky, not as Earth's ephemeris.
- The Earth frame is a model frame. Turning it into the real ITRF would need UT1 that varies with time, nutation and polar motion (Kaplan 2005).
- In deep time the epoch only anchors the phases, and the geological age sets the rates.

## 6. Contracts

### S4: climate and atmosphere

- Make one `Instant` per climate step: `epoch` fixed per world (J2000 unless the world chooses a date), `seconds` from the session clock, `age` from `Calendar::era`. Call `skyAt` once per step. The `Sky` is a value, so share it, don't rebuild it per cell.
- Per cell, take the cell's `up` from S1:
  - `insolation(sky, up)` in W/m² above the air replaces `dailyInsolation(latitude, declination)` × solar constant;
  - average it over the step's samples, or integrate a day by sampling `skyAt` (about 0.6 µs a call);
  - `sunElevation(sky, up)` replaces `sunDirectionAt`'s height, and `solarTime(sky, up)` replaces `hourOfDay` (local, so it varies with longitude);
  - `sunDeclination(sky)` replaces `declinationOn`.
- Seasons, 1/r² and polar night come out of these with no switch. Day length is where `sunElevation` crosses the air's own horizon (refraction is the air's).
- Coriolis is S1's and S4's: f = 2Ω sin φ with Ω = 2π / `sky.deep.siderealDay` (7.292 × 10⁻⁵ s⁻¹ today; it changes with age).
- The tide (S3, S4 or the coast) is a force that moves water and makes none.
  - The sea takes `tidePotential` as the forcing of its own flow, or, in the equilibrium approximation, `wetTide` over its wet cells with their true areas, × 0.69 against the yielding sea floor. Either replaces `tideHeight`.
  - Never add `equilibriumTide` to a sea level. Its mean is zero only over the whole sphere. Over an ocean it is not: +0.20 m over the cap z ≥ 0.5 with both bodies overhead.
  - With a fixed mask, `wetTide` subtracts the area-weighted mean over the wet cells, which is enough.
  - Where cells wet and dry with the tide, the common level correction comes from the volume over the real bathymetry. That is the sea owner's equilibrium solve: find the level at which the volume filled equals the volume held.
  - S4's or the coast's closed-budget test must include an uneven wet mask, and preferably cells that wet and dry. A uniform or whole-sphere sea passes even with the wrong contract.
  - The dynamic response is later work.
- Deep time: `sky.deep` carries the day, month and luminosity of the age. A climate that runs through geological time calls `deepTimeAt(age)` per epoch, without building a sky.

### S6: engine, bridge, game

- The game clock is `seconds` of one `Instant` per world, so pause, time scale and jumps are just a different `seconds`. Keep `epoch` and `age` in the world's identity (save files).
- The bridge carries per frame, in the Earth frame:
  - `sunDirection(sky)` and `moonDirection(sky, observer)`, where the observer is the camera's ECEF position from S1;
  - `irradiance(sky)`;
  - `sunCoverage(sky, observer)`;
  - `moonIllumination(sky, observer)`, `moonSunward(sky)` and `lunarShadow(sky, PlanetRadius).umbralCoverage`.
  The renderer turns these into its local frame through S1's tangent frame.

### Renderer: the Sun and Moon bodies (decision 6)

- **Sun disc**
  - Centre at `sunDirection`, angular radius `sunAngularRadius(sky)` (0.262° to 0.271° over the year), limb darkening the renderer's.
  - Its light is `irradiance(sky)` × (1 − `sunCoverage`) through the air model.
- **Moon disc**
  - Centre at `moonDirection(sky, camera)`, angular radius `moonAngularRadius(sky, camera)` (0.245° to 0.279°).
  - Light it as a sphere from `moonSunward(sky)`, so the phase follows from the geometry; earthshine on the dark side from the Earth's lit share.
  - Dim it by `umbralCoverage` during a lunar eclipse; the red of the umbra is the air's.
  - Moonlight is its own light source: irradiance from the Sun's at the Moon × the Moon's albedo × the lit share (`moonIllumination`), about 0.25 lx at full moon.
- No texture carries a painted Sun or Moon.

## 7. How to verify

```sh
cd @simulation && X=$(ls -d ~/.local/share/mise/installs/github-xmake-io-xmake/*/ | head -1)xmake
XMAKE_CONFIGDIR=$PWD/../.out/astronomy/xmake $X config --yes --mode=release --builddir=../.out/astronomy/build && XMAKE_CONFIGDIR=$PWD/../.out/astronomy/xmake $X build --yes simulation-tests
../.out/astronomy/build/macosx/arm64/release/simulation-tests astronomy
```

Inside `@simulation` the `xmake` mise shim sets `XMAKE_CONFIGDIR` to the shared `.out/simulation/xmake` (`mise.toml` `[env]`) and overrides your own. A private build therefore calls the xmake binary directly, as above.

The reference values are written in the tests with their source and frame. The Horizons queries used to fetch them are kept in `.out/astronomy/horizons/` (`fetch.sh`, `rise.sh`, `aps.sh`, `standstill.sh`).

Only macOS (arm64, release) has been run.

## 8. Open

- When S4 and S6 have moved the consumers, delete `Calendar.hpp`'s `hourOfDay`, `declinationOn`, `lunarPhase`, `tideHeight` and their constants (S7).
- Two facts still have two owners. `PresentAge` repeats `World::age{4.6e9}` (`simulation/type/World.hpp`), so S6 should make `World` use it. `JulianDay` repeats `Calendar.hpp`'s `DaySeconds`, which S7 deletes.
- A daily-mean insolation for an `up`, in closed form, would spare S4's coarse steps the sampling. Add it when S4 asks.
- The brief's "13.1 sidereal months a year at 620 Ma" is Williams' 13.1 synodic months (14.1 sidereal). The tests check synodic.
- The deep-time day leaves out the solar tidal braking. Adding it would move 620 Ma from 22.09 h towards the record's 21.9 h; it is within the record's error already.
- The early Moon (before 2450 Ma) has no record here. If the world's early history needs a close Moon (strong tides, a short day), that is a separate model choice.
- Milankovitch pacing of the glacial cycles is S4b.

## 9. Sources

Credited per AGENTS.md; the credits-sweep session moves them into `docs/credits.md`. The code cites each by author and year where it applies it.

- **Meeus, J. (1998).** *Astronomical Algorithms*, 2nd ed. Willmann-Bell, Richmond VA. ISBN 0-943396-61-1. Informs:
  - the mean elements of the Sun and the Moon at J2000 and their rates (ch. 25, 47; the Earth's semi-major axis from table 31.A), from which the tropical, sidereal and anomalistic years follow;
  - the Moon's non-Keplerian terms (tables 47.A and 47.B);
  - Greenwich mean sidereal time (12.4);
  - the equation of time and its test value (ch. 28, example 28.b).
- **Lieske, J. H., Lederle, T., Fricke, W., Morando, B. (1977).** "Expressions for the precession quantities based upon the IAU (1976) system of astronomical constants." *Astronomy and Astrophysics* 58, 1–16. Informs: the general precession in longitude (5029.0966″ per century) that the tropical and sidereal years imply.
- **Park, R. S., Folkner, W. M., Williams, J. G., Boggs, D. H. (2021).** "The JPL Planetary and Lunar Ephemerides DE440 and DE441." *The Astronomical Journal* 161, 105. doi:10.3847/1538-3881/abd414. Informs: every Horizons reference value in the tests (DE441), and the Moon's GM.
- **Giorgini, J. D., et al. (1996).** "JPL's On-Line Solar System Data Service." *Bulletin of the American Astronomical Society* 28(3), 1158; https://ssd.jpl.nasa.gov/horizons/ (frames and time scales: https://ssd.jpl.nasa.gov/horizons/manual.html; API: https://ssd-api.jpl.nasa.gov/doc/horizons.html). Informs: the test positions, elevations, sunrises, apsides and lunar reach, queried through its API (`.out/astronomy/horizons/*.sh`).
- **Williams, G. E. (2000).** "Geological constraints on the Precambrian history of Earth's rotation and the Moon's orbit." *Reviews of Geophysics* 38(1), 37–59. doi:10.1029/1999RG900016. Informs: the deep-time Moon distance (0.965 a₀ at 620 Ma; 1.24 cm/yr from 2450 to 620 Ma) and the test values at 620 Ma (21.9 h, 400 days, 13.1 synodic months a year).
- **Dickey, J. O., Bender, P. L., Faller, J. E., et al. (1994).** "Lunar Laser Ranging: A Continuing Legacy of the Apollo Program." *Science* 265, 482–490. doi:10.1126/science.265.5171.482. Informs: today's lunar recession of 3.82 cm/yr, the start of the future extrapolation.
- **Gough, D. O. (1981).** "Solar interior structure and luminosity variations." *Solar Physics* 74, 21–34. doi:10.1007/BF00151270. Informs: the Sun's luminosity over its main-sequence life, L = L☉ / (1 + 0.4 (1 − t/t☉)).
- **Murray, C. D., Dermott, S. F. (1999).** *Solar System Dynamics*. Cambridge University Press. doi:10.1017/CBO9781139174817. Informs:
  - the constant-Q tidal recession (ȧ ∝ a^−11/2) of the future Moon;
  - the solar driving of the Moon's nodal and apsidal precession (∝ n☉² / n☾) used to scale their periods with age.
- **Yoder, C. F. (1995).** "Astrometric and Geodetic Properties of Earth and the Solar System." In *Global Earth Physics: A Handbook of Physical Constants*, AGU Reference Shelf 1, 1–31. doi:10.1029/RF001p0001. Informs: the Earth's polar moment of inertia (C = 0.3307 M R², 8.034 × 10³⁷ kg m²) in the angular-momentum day.
- **Laskar, J., Robutel, P., Joutel, F., Gastineau, M., Correia, A. C. M., Levrard, B. (2004).** "A long-term numerical solution for the insolation quantities of the Earth." *Astronomy and Astrophysics* 428, 261–285. doi:10.1051/0004-6361:20041335.
- **Laskar, J., Fienga, A., Gastineau, M., Manche, H. (2011).** "La2010: a new orbital solution for the long-term motion of the Earth." *Astronomy and Astrophysics* 532, A89. doi:10.1051/0004-6361/201116836.
  - Both Laskar papers inform the stated limit: the orbital cycles are not an exact ephemeris beyond about 50 Myr, so they are left to S4b.
- **Prša, A., et al. (2016).** "Nominal Values for Selected Solar and Planetary Quantities: IAU 2015 Resolution B3." *The Astronomical Journal* 152, 41. doi:10.3847/0004-6256/152/2/41. Informs: the Sun's luminosity (3.828 × 10²⁶ W), radius (6.957 × 10⁸ m) and GM, and the 1361 W/m² checked at 1 AU.
- **International Astronomical Union (2012).** Resolution B2, "On the re-definition of the astronomical unit of length." XXVIII General Assembly, Beijing. https://www.iau.org/. Informs: 1 AU = 149 597 870 700 m.
- **Petit, G., Luzum, B. (eds.) (2010).** *IERS Conventions (2010)*. IERS Technical Note 36, Verlag des Bundesamts für Kartographie und Geodäsie, Frankfurt am Main. https://www.iers.org/IERS/EN/Publications/TechnicalNotes/tn36.html. Informs: the Earth's GM (3.986004418 × 10¹⁴ m³/s²) and the Love numbers behind the 0.69 factor of the observed equilibrium tide.
- **IERS / U.S. Naval Observatory**, ΔT and Earth-orientation data. https://maia.usno.navy.mil/products/deltaT. Informs: TT − UT1 held at 69.2 s.
- **Tiesinga, E., Mohr, P. J., Newell, D. B., Taylor, B. N. (2021).** "CODATA recommended values of the fundamental physical constants: 2018." *Reviews of Modern Physics* 93, 025010. doi:10.1103/RevModPhys.93.025010. Informs: G.
- **Cartwright, D. E., Tayler, R. J. (1971).** "New Computations of the Tide-generating Potential." *Geophysical Journal of the Royal Astronomical Society* 23(1), 45–73. doi:10.1111/j.1365-246X.1971.tb01803.x. Informs: the tide-generating potential's expansion in degrees 2 and 3.
- **Espenak, F., Meeus, J. (2006).** *Five Millennium Canon of Solar Eclipses: −1999 to +3000*. NASA/TP-2006-214141; and **Espenak, F., Meeus, J. (2009).** *Five Millennium Canon of Lunar Eclipses: −1999 to +3000*. NASA/TP-2009-214172. https://eclipse.gsfc.nasa.gov/. Catalogues: https://eclipse.gsfc.nasa.gov/SEcat5/SE2001-2100.html and https://eclipse.gsfc.nasa.gov/LEcat5/LE2001-2100.html. Magnitudes and greatest-eclipse times (TD) as listed on https://eclipsewise.com/. Informs:
  - the eclipse dates, kinds, γ and umbral magnitudes of 2024–2025 in the tests;
  - the fundamental-plane description of the Moon's shadow.
- **Chauvenet, W. (1863).** *A Manual of Spherical and Practical Astronomy*, vol. 1. J. B. Lippincott, Philadelphia. Informs: the 2 % enlargement of the Earth's shadow for its air (https://eclipse.gsfc.nasa.gov/LEcat5/shadow.html). NASA's older canons use it; the Five Millennium lunar canon uses Danjon's rule. Our umbral magnitudes agree with the Five Millennium canon's to 0.04, within the tests' 0.1.
- **Urban, S. E., Seidelmann, P. K. (eds.) (2013).** *Explanatory Supplement to the Astronomical Almanac*, 3rd ed. University Science Books, Mill Valley CA. Informs: the −0.8333° convention for sunrise (34′ refraction plus 16′ semidiameter) in the tests.
- **U.S. Naval Observatory**, "Earth's Seasons and Apsides." https://aa.usno.navy.mil/data/Earth_Seasons. Informs: the 2024 and 2025 March equinoxes in the tests.
- **Kaplan, G. H. (2005).** *The IAU Resolutions on Astronomical Reference Systems, Time Scales, and Earth Rotation Models: Explanation and Implementation*. U.S. Naval Observatory Circular 179. https://aa.usno.navy.mil/downloads/Circular_179.pdf. Informs: what the model's Earth frame leaves out against the real ITRF (UT1 that varies with time, nutation, polar motion) and how UT1 enters sidereal time.
- **González, Á. (2010).** "Measurement of areas on a sphere using Fibonacci and latitude–longitude lattices." *Mathematical Geosciences* 42, 49–64. doi:10.1007/s11004-009-9257-x. Informs: the even sphere sampling of the tide test.
