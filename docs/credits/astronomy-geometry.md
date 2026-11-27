# Credits: astronomy and the geometry of the sphere

Part of [Credits](../credits.md); the entry format is explained there. Both domains are new (planet slices S1 and S2, 26 Nov 2026) and still in progress; the entries follow the Sources sections of `docs/handoff/geometry.md` (S1, commit `cb87997`; its lines and the geometry source lines as of S1's follow-up to Codex's DREV-CLAUDE-002-03) and `docs/handoff/astronomy.md:184` (S2, commits `e3ed117`, `56c2ef1` and the tide's one-body contract), with the bibliographic data checked again here. Astronomy line numbers are as of the one-body tide contract (DREV-CLAUDE-002-04).

## The Sun, the Earth and the Moon today

### Meeus 1998: Astronomical Algorithms
Meeus, J. (1998). *Astronomical Algorithms*, 2nd ed. Willmann-Bell, Richmond, VA. ISBN 0-943396-61-1.
- **Model:** the mean elements of the Sun and the Moon at J2000 and their rates (ch. 25, 47; the Earth's semi-major axis from table 31.A), from which the tropical, sidereal and anomalistic years follow; the Moon's non-Keplerian terms (tables 47.A and 47.B down to 0.01°); Greenwich mean sidereal time (eq. 12.4); the equation of time (ch. 28).
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:9`, `:26`, `:43`, `:50`; `@simulation/src/astronomy/fn/Orbit.hpp:31`, `@simulation/src/astronomy/fn/Orbit.cpp:25`; `@simulation/src/astronomy/fn/Frame.hpp:21`; `@simulation/src/astronomy/fn/Place.hpp:28`.
- **Checked by:** `@simulation/test/Astronomy.cpp:171`, `:183`, the equation of time against his example 28.b.
- **Approximations:** the truncated lunar series leaves out terms below 0.01° (0.046° at most against Horizons), and sin(M′ − 2F) comes out of the inclined plane instead of the table (`Orbit.cpp:28`; `@simulation/test/Astronomy.cpp:51`).

### Lieske et al. 1977: the IAU 1976 precession
Lieske, J. H., Lederle, T., Fricke, W., Morando, B. (1977). Expressions for the precession quantities based upon the IAU (1976) system of astronomical constants. *Astronomy and Astrophysics* 58, 1–16. https://ui.adsabs.harvard.edu/abs/1977A%26A....58....1L
- **Model:** the general precession in longitude, 5029.0966″ per century, which sets the tropical against the sidereal year.
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:27`.

### IAU 2012 Resolution B2 and IAU 2015 Resolution B3: nominal values
International Astronomical Union (2012). Resolution B2, on the re-definition of the astronomical unit of length (1 au = 149 597 870 700 m). XXVIII General Assembly, Beijing. https://www.iau.org/ (unverified: stable URL of the resolution text; `docs/handoff/astronomy.md` gives one that no longer resolves)

Prša, A., Harmanec, P., Torres, G., et al. (2016). Nominal values for selected solar and planetary quantities: IAU 2015 Resolution B3. *The Astronomical Journal* 152(2), 41. doi:10.3847/0004-6256/152/2/41
- **Model:** the astronomical unit; the Sun's nominal luminosity 3.828 × 10²⁶ W, radius 6.957 × 10⁸ m and GM, and so 1361 W/m² at 1 au.
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:6`, `:7`, `:22`.
- **Checked by:** `@simulation/test/Astronomy.cpp:128`, `:163`, 1/r² and 1361 W/m² at 1 au.

### Petit and Luzum 2010: IERS Conventions
Petit, G., Luzum, B. (eds.) (2010). *IERS Conventions (2010)*. IERS Technical Note 36. Verlag des Bundesamts für Kartographie und Geodäsie, Frankfurt am Main. ISBN 3-89888-989-6. https://iers-conventions.obspm.fr/content/tn36.pdf
- **Model:** the Earth's GM (3.986004418 × 10¹⁴ m³/s²) and the Love numbers behind the factor (1 + k₂ − h₂) = 0.69 of the observed equilibrium tide.
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:8`; `@simulation/src/astronomy/fn/Tide.hpp:22`.

### IERS and U.S. Naval Observatory: ΔT
IERS / U.S. Naval Observatory, ΔT and Earth-orientation data. https://maia.usno.navy.mil/products/deltaT
- **Reference data:** TT − UT1 held at its 2024–2026 value of 69.2 s.
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:43`; `@simulation/src/astronomy/fn/Frame.hpp:22`.
- **Approximations:** ΔT held fixed (1987 had 55.6 s, about 0.06° of the Moon; `@simulation/test/Astronomy.cpp:54`).

### Tiesinga et al. 2021: CODATA 2018
Tiesinga, E., Mohr, P. J., Newell, D. B., Taylor, B. N. (2021). CODATA recommended values of the fundamental physical constants: 2018. *Reviews of Modern Physics* 93, 025010. doi:10.1103/RevModPhys.93.025010
- **Model:** the gravitational constant 6.6743 × 10⁻¹¹ m³ kg⁻¹ s⁻².
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:16`.

### Yoder 1995: the Earth's moment of inertia
Yoder, C. F. (1995). Astrometric and geodetic properties of Earth and the solar system. In: Ahrens, T. J. (ed.), *Global Earth Physics: A Handbook of Physical Constants*, AGU Reference Shelf 1, 1–31. doi:10.1029/RF001p0001
- **Model:** the polar moment of inertia C = 0.3307 M R² (8.034 × 10³⁷ kg m²) in the angular-momentum day.
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:8`; `@simulation/src/astronomy/fn/DeepTime.cpp:42`.

### Cartwright and Tayler 1971: the tide-generating potential
Cartwright, D. E., Tayler, R. J. (1971). New computations of the tide-generating potential. *Geophysical Journal of the Royal Astronomical Society* 23(1), 45–73. doi:10.1111/j.1365-246X.1971.tb01803.x
- **Model:** the expansion of the tide-generating potential in degrees 2 and 3; the equilibrium tide. The tide is a force that moves water: over a fixed wet mask, the planet-wide shape is shifted by its area-weighted mean over the wet cells, so the volume does not change (`wetTide`).
- **Used in:** `@simulation/src/astronomy/fn/Tide.hpp:11`, `:37`.
- **Checked by:** `@simulation/test/AstronomyTide.cpp:20`, `:49`: zero mean over the whole sphere; +0.1956 m over the cap z ≥ 0.5 with both bodies overhead (Codex, DREV-CLAUDE-002-03), brought to zero volume over that cap and over two uneven masks at two instants, the second larger.
- **Limits:** the ocean's dynamic response is later work. Where the coast follows the bathymetry, the common level correction comes from the volume over the real bathymetry, owned by the sea (`Tide.hpp:31`).

### Chauvenet 1863: the enlarged shadow of the Earth
Chauvenet, W. (1863). *A Manual of Spherical and Practical Astronomy*, vol. 1. J. B. Lippincott, Philadelphia; 5th ed. 1891 (Dover reprint 1960), the edition NASA cites. https://eclipse.gsfc.nasa.gov/LEcat5/shadow.html
- **Model:** the Earth's shadow at the Moon enlarged by about 2 % for its air.
- **Used in:** `@simulation/src/astronomy/fn/Eclipse.hpp:29`.
- **Note:** NASA's Five Millennium lunar canon enlarges the shadow by Danjon's rule, not Chauvenet's; `Eclipse.hpp:29` names Chauvenet's rule as that of NASA's older canons.

## Measured sky the model is tested against

### Giorgini et al. 1996; Park et al. 2021: JPL Horizons and DE441
Giorgini, J. D., Yeomans, D. K., Chamberlin, A. B., et al. (1996). JPL's on-line solar system data service. *Bulletin of the American Astronomical Society* 28(3), 1158. https://ssd.jpl.nasa.gov/horizons/; frames and time scales: https://ssd.jpl.nasa.gov/horizons/manual.html; API: https://ssd-api.jpl.nasa.gov/doc/horizons.html

Park, R. S., Folkner, W. M., Williams, J. G., Boggs, D. H. (2021). The JPL planetary and lunar ephemerides DE440 and DE441. *The Astronomical Journal* 161(3), 105. doi:10.3847/1538-3881/abd414
- **Reference data:** geocentric astrometric RA/Dec and distances, topocentric airless elevations, sunrises, the apsides of 2024 and the Moon's reach in 2015 and 2025, queried 26 Nov 2026 (`.out/astronomy/horizons/*.sh`); the Moon's GM.
- **Used in:** `@simulation/src/astronomy/constant/Sky.hpp:8`.
- **Checked by:** `@simulation/test/Astronomy.cpp:1`, `:20`, `:35`, `:59`, `:84`, `:153`; `@simulation/test/AstronomyEvents.cpp:113`, `:134`; `@simulation/test/AstronomyFixture.hpp:5`.

### Espenak and Meeus 2006, 2009: the NASA eclipse canons
Espenak, F., Meeus, J. (2006). *Five Millennium Canon of Solar Eclipses: −1999 to +3000*. NASA/TP-2006-214141. https://eclipse.gsfc.nasa.gov/SEpubs/5MCSE.html

Espenak, F., Meeus, J. (2009). *Five Millennium Canon of Lunar Eclipses: −1999 to +3000*. NASA/TP-2009-214172. https://eclipse.gsfc.nasa.gov/SEpubs/5MCLE.html
- **Model:** the description of the Moon's shadow on the fundamental plane.
- **Reference data:** the eclipse greatest-phase times (TD), kinds, γ and umbral magnitudes of 2024–2025, from the catalogues https://eclipse.gsfc.nasa.gov/SEcat5/SE2001-2100.html and https://eclipse.gsfc.nasa.gov/LEcat5/LE2001-2100.html (magnitudes and times as listed on https://eclipsewise.com/). Only these are independent; the shadow's centre and totality from the same sky are consistency checks.
- **Used in:** `@simulation/src/astronomy/fn/Eclipse.hpp:9`.
- **Checked by:** `@simulation/test/AstronomyEvents.cpp:1`.

### U.S. Naval Observatory: seasons and the sunrise convention
U.S. Naval Observatory. Earth's seasons and apsides. https://aa.usno.navy.mil/data/Earth_Seasons

Urban, S. E., Seidelmann, P. K. (eds.) (2013). *Explanatory Supplement to the Astronomical Almanac*, 3rd ed. University Science Books, Mill Valley, CA. ISBN 978-1-891389-85-6.
- **Reference data:** the March equinoxes of 2024 and 2025; sunrise and sunset when the Sun's centre is at −0.8333° (34′ refraction and 16′ semidiameter).
- **Checked by:** `@simulation/test/Astronomy.cpp:85`, `:101`, `:117`, `:127`.

### González 2010: even points on a sphere
González, Á. (2010). Measurement of areas on a sphere using Fibonacci and latitude–longitude lattices. *Mathematical Geosciences* 42(1), 49–64. doi:10.1007/s11004-009-9257-x
- **Model:** Fibonacci points that cover the sphere evenly, for sampling the tide.
- **Checked by:** `@simulation/test/AstronomyTide.cpp:25`.
- **Used in (S3b):** the plate sites of an epoch on the sphere, a jittered Fibonacci lattice, `@simulation/src/tectonics/fn/Euler.hpp:8`, `@simulation/src/tectonics/fn/Euler.cpp:51`.

### Kaplan 2005: the IAU reference systems and Earth rotation
Kaplan, G. H. (2005). *The IAU Resolutions on Astronomical Reference Systems, Time Scales, and Earth Rotation Models: Explanation and Implementation*. U.S. Naval Observatory Circular 179. https://aa.usno.navy.mil/downloads/Circular_179.pdf
- **Model:** how UT1 enters sidereal time, and what the model's Earth frame leaves out against the real ITRF: UT1 that varies with time, nutation and polar motion.
- **Limits:** the model frame is checked against Earth only from 1987 to 2026 (`docs/handoff/astronomy.md`, Range and accuracy).

## Deep time

### Williams 2000: day length and the Moon's distance from tidal rhythmites
Williams, G. E. (2000). Geological constraints on the Precambrian history of Earth's rotation and the Moon's orbit. *Reviews of Geophysics* 38(1), 37–59. doi:10.1029/1999RG900016
- **Model:** the Moon's distance before today from the rhythmite record (0.965 a₀ at 620 Ma, 1.24 cm/yr from 2450 to 620 Ma); the day from conserving the Earth–Moon angular momentum.
- **Used in:** `@simulation/src/astronomy/constant/DeepTime.hpp:10`; `@simulation/src/astronomy/type/DeepTime.hpp:10`.
- **Checked by:** `@simulation/test/AstronomyEvents.cpp:144`, `:156`–`:162`: at 620 Ma a day of 21.9 ± 0.4 h, 400 ± 7 days and 13.1 synodic months a year.

### Dickey et al. 1994; Murray and Dermott 1999: the Moon's recession and precessions
Dickey, J. O., Bender, P. L., Faller, J. E., et al. (1994). Lunar laser ranging: a continuing legacy of the Apollo program. *Science* 265(5171), 482–490. doi:10.1126/science.265.5171.482

Murray, C. D., Dermott, S. F. (1999). *Solar System Dynamics*. Cambridge University Press. doi:10.1017/CBO9781139174817
- **Model:** today's recession of 3.82 cm/yr, extrapolated into the future under a constant tidal Q (ȧ ∝ a^−11/2); the Sun's driving of the Moon's nodal and apsidal precession (∝ n☉²/n☾), which scales their periods with age.
- **Used in:** `@simulation/src/astronomy/constant/DeepTime.hpp:13`; `@simulation/src/astronomy/fn/DeepTime.cpp:37`.

### Gough 1981: the faint young Sun
Gough, D. O. (1981). Solar interior structure and luminosity variations. *Solar Physics* 74(1), 21–34. doi:10.1007/BF00151270
- **Model:** L = L☉ / (1 + 0.4 (1 − t/t☉)).
- **Used in:** `@simulation/src/astronomy/constant/DeepTime.hpp:21`, `:24`; `@simulation/src/astronomy/fn/DeepTime.cpp:50`; `@simulation/src/astronomy/type/DeepTime.hpp:12`.
- **Checked by:** `@simulation/test/AstronomyEvents.cpp:146`: the young Sun gives 1/1.4 of today's light.

### Laskar et al. 2004, 2011; Milankovitch 1941: orbital cycles (planned, S4b)
Laskar, J., Robutel, P., Joutel, F., et al. (2004). A long-term numerical solution for the insolation quantities of the Earth. *Astronomy & Astrophysics* 428(1), 261–285. doi:10.1051/0004-6361:20041335

Laskar, J., Fienga, A., Gastineau, M., Manche, H. (2011). La2010: a new orbital solution for the long-term motion of the Earth. *Astronomy & Astrophysics* 532, A89. doi:10.1051/0004-6361/201116836

Milankovitch, M. (1941). *Kanon der Erdbestrahlung und seine Anwendung auf das Eiszeitenproblem*. Königlich Serbische Akademie, Belgrade. (unverified: publisher series)
- **Model:** eccentricity, obliquity and precession cycles (41 kyr, 19/23 kyr, 100/405 kyr) pacing the ice; La2010 is an exact ephemeris only to about 50 Myr, so they are left as cycles to S4b.
- **Used in:** `@simulation/src/astronomy/type/DeepTime.hpp:16`, `:17` (why they are not there yet); `docs/handoff/astronomy.md:58`, `:205`; `.out/lead/handover/PLANET.md:77`.

## The sphere and its grid

### Google S2 Geometry
Veach, E., et al. (2005 onwards). *S2 Geometry*, C++ library, Google, Apache-2.0. https://github.com/google/s2geometry; cell hierarchy: https://s2geometry.io/devguide/s2cell_hierarchy.html
- **Model:** the face numbering and orientation, the tie rule between faces, the ST/UV split and the face quadtree. The library itself is not linked (`docs/handoff/geometry.md:56`).
- **Used in:** `@simulation/src/geometry/constant/Sphere.hpp:15`, `:32`; `@simulation/src/geometry/fn/Projection.hpp:16`, `:20`; `@simulation/src/geometry/fn/Neighbour.hpp:12`.
- **Checked by:** `@simulation/test/Geometry.cpp:139`, S2's face tie rule.

### Sadourny 1972; Ronchi, Iacono and Paolucci 1996: the gnomonic and equiangular cubed sphere
Sadourny, R. (1972). Conservative finite-difference approximations of the primitive equations on quasi-uniform spherical grids. *Monthly Weather Review* 100(2), 136–144. doi:10.1175/1520-0493(1972)100<0136:CFAOTP>2.3.CO;2

Ronchi, C., Iacono, R., Paolucci, P. S. (1996). The "cubed sphere": a new method for the solution of partial differential equations in spherical geometry. *Journal of Computational Physics* 124(1), 93–114. doi:10.1006/jcph.1996.0047
- **Model:** the gnomonic cube grid, and its equiangular mapping u = tan(π/4 · (2s − 1)).
- **Used in:** `@simulation/src/geometry/fn/Projection.hpp:9`; `docs/handoff/geometry.md:13`, `:97`, `:98`.

### Rančić, Purser and Mesinger 1996; Putman and Lin 2007; Lin 2004: why gnomonic, why equiangular
Rančić, M., Purser, R. J., Mesinger, F. (1996). A global shallow-water model using an expanded spherical cube: gnomonic versus conformal coordinates. *Quarterly Journal of the Royal Meteorological Society* 122(532), 959–982. doi:10.1002/qj.49712253209

Putman, W. M., Lin, S.-J. (2007). Finite-volume transport on various cubed-sphere grids. *Journal of Computational Physics* 227(1), 55–78. doi:10.1016/j.jcp.2007.07.022

Lin, S.-J. (2004). A "vertically Lagrangian" finite-volume dynamical core for global models. *Monthly Weather Review* 132(10), 2293–2307. doi:10.1175/1520-0493(2004)132<2293:AVLFDC>2.0.CO;2; GFDL FV3: https://www.gfdl.noaa.gov/fv3/
- **Model:** great-circle cell edges and orthonormal vector components; the equiangular grid FV3 uses, which the atmosphere slice S4 will follow; the grid's non-orthogonality, why a flux crosses an edge along the edge's own normal rather than the centre-to-centre direction.
- **Used in:** `@simulation/src/geometry/fn/Edge.hpp:7`; `docs/handoff/geometry.md:20`, `:52`, `:91`, `:99`, `:100`; `.out/lead/handover/PLANET.md:11`, `:371`.
- **Used in (atmosphere on the sphere):** the dual-basis edge gradient, `@simulation/src/atmosphere/type/SphereStencil.hpp:17`; `docs/handoff/atmosphere-sphere.md:16`.

### O'Neill and Laubscher 1976; PROJ: QSC (considered, not taken)
O'Neill, E. M., Laubscher, R. E. (1976). *Extended studies of a quadrilateralized spherical cube Earth data base*. Naval Environmental Prediction Research Facility, Tech. Report NEPRF 3-76 (CSC). PROJ implementation: https://proj.org/en/stable/operations/projections/qsc.html
- **Model:** the equal-area cube; rejected for worse angles at the face edges and no great-circle cell edges.
- **Used in:** `docs/handoff/geometry.md:101`; `.out/lead/handover/PLANET.md:41`.

### van Oosterom and Strackee 1983: area of a spherical triangle
van Oosterom, A., Strackee, J. (1983). The solid angle of a plane triangle. *IEEE Transactions on Biomedical Engineering* BME-30(2), 125–126. doi:10.1109/TBME.1983.325207
- **Model:** the tan(E/2) formula for spherical excess; cell area as excess × R².
- **Used in:** `@simulation/src/geometry/fn/Metric.hpp:10`, `@simulation/src/geometry/fn/Metric.cpp:35`.
- **Used in (S3a):** the area of a polygon and of an overlap as the fan's excesses from plane coordinates, `@simulation/src/geometry/fn/Overlap.hpp:12`, `@simulation/src/geometry/fn/Overlap.cpp:88`.
- **Used in (S3b):** the signed area of each contour of a union of footprints, outer contours positive and holes negative, `@simulation/src/geometry/fn/Union.hpp:20`, `@simulation/src/geometry/fn/Union.cpp:32`.

### Rodrigues 1840; do Carmo 1976: rotation and holonomy
Rodrigues, O. (1840). Des lois géométriques qui régissent les déplacements d'un système solide dans l'espace… *Journal de Mathématiques Pures et Appliquées* 5, 380–440. http://www.numdam.org/item/JMPA_1840_1_5__380_0/

do Carmo, M. P. (1976). *Differential Geometry of Curves and Surfaces*. Prentice-Hall, §4-5 (Gauss–Bonnet).
- **Model:** the rotation about from × to that carries a vector along, in Rodrigues' unit-axis form beyond a right angle so it stays finite up to the antipode; parallel transport round a loop turns by area / R².
- **Used in:** `@simulation/src/geometry/fn/Frame.hpp:33`, `:38`; `@simulation/src/geometry/fn/Frame.cpp:62`.
- **Checked by:** `@simulation/test/GeometryFrame.cpp:77` (finite and reversible near and at the antipode), `:154` (holonomy against the enclosed solid angle).
- **Used in (S3a):** a rotation as its Euler–Rodrigues parameters (cos θ/2, sin θ/2 · axis), the unit quaternion of `@simulation/src/geometry/type/Rotation.hpp:8`.

### Hamilton 1844: quaternions
Hamilton, W. R. (1844). On quaternions; or on a new system of imaginaries in algebra. *The London, Edinburgh, and Dublin Philosophical Magazine and Journal of Science* 25(163), 10–13. doi:10.1080/14786444408644923
- **Model:** the quaternion product, which composes rotations; the conjugate as the inverse of a unit quaternion.
- **Used in:** `@simulation/src/geometry/type/Rotation.hpp:7`; `@simulation/src/geometry/fn/Rotation.hpp:20`, `@simulation/src/geometry/fn/Rotation.cpp:23`.
- **Checked by:** `@simulation/test/GeometrySphere.cpp:95`: composition order, inverse, matrix, 2π in 10³ and 10⁵ steps back to 1e-12, 10⁶ steps of 1e-9 rad equal to one of 1e-3 to 5e-14.

### Sinnott 1984: haversine distance
Sinnott, R. W. (1984). Virtues of the haversine. *Sky and Telescope* 68(2), 159.
- **Checked by:** `@simulation/test/GeometryFrame.cpp:168`, the independent distance `distanceBetween` is compared with (`docs/handoff/geometry.md:105`).

### Karney 2013: ellipsoidal geodesics (for later)
Karney, C. F. F. (2013). Algorithms for geodesics. *Journal of Geodesy* 87, 43–55. doi:10.1007/s00190-012-0578-z
- **Model:** the geodesic reference (GeographicLib) to adopt once flattening is modelled; not used on the sphere (`docs/handoff/geometry.md:106`).

### Moritz 2000; NIMA 2000: Earth's radius and flattening
Moritz, H. (2000). Geodetic Reference System 1980. *Journal of Geodesy* 74, 128–133. doi:10.1007/s001900050278

National Imagery and Mapping Agency (1997, amended 2000). *Department of Defense World Geodetic System 1984*. TR 8350.2, 3rd ed. https://earth-info.nga.mil/GandG/wgs84/
- **Model:** the mean radius R₁ = 6 371 008.8 m, rounded to 6 371 000 m; the flattening 1/298.257223563 recorded as the known deviation.
- **Used in:** `@simulation/src/geometry/constant/Sphere.hpp:7`, `:9`, `:13`.

### Morton 1966: the Z-order key
Morton, G. M. (1966). *A computer oriented geodetic data base and a new technique in file sequencing*. International Business Machines Co., Ottawa.
- **Model:** the interleave of i and j bits in the cell key.
- **Used in:** `@simulation/src/geometry/constant/Sphere.hpp:33`, `:34`; `docs/handoff/geometry.md:14`.

### Williamson et al. 1992: shallow-water test cases on the sphere (planned, S4)
Williamson, D. L., Drake, J. B., Hack, J. J., Jakob, R., Swarztrauber, P. N. (1992). A standard test set for numerical approximations to the shallow water equations in spherical geometry. *Journal of Computational Physics* 102(1), 211–224. doi:10.1016/S0021-9991(05)80016-6
- **Model:** the standard test cases the atmosphere's operator on the cubed sphere is to pass, after the degree-1 harmonic and the closed-mesh divergence sum.
- **Used in:** planned, `docs/handoff/geometry.md:90`, `:109`.
- **Checked by (atmosphere on the sphere):** case 1 (cosine bell) `@simulation/test/AtmosphereSphereFlow.cpp:25`, `:149`; case 6 (Rossby–Haurwitz wave) `@simulation/test/AtmosphereSphereFixture.hpp:57`; cases 2 and 5 need the shallow-water equations (`docs/handoff/atmosphere-sphere.md:142`).

### Jones 1999: conservative remapping
Jones, P. W. (1999). First- and second-order conservative remapping schemes for grids in spherical coordinates. *Monthly Weather Review* 127(9), 2204–2210. doi:10.1175/1520-0493(1999)127<2204:FASOCR>2.0.CO;2
- **Model:** moving extensive stocks between grids by intersecting spherical areas.
- **Used in:** planned, `.out/lead/handover/PLANET.md:207`.
- **Used in (S3a):** the overlap of two convex geodesic polygons, the weight of a crust piece's footprint in a mantle cell, `@simulation/src/geometry/fn/Overlap.hpp:7`.
- **Checked by:** `@simulation/test/GeometrySphereOverlap.cpp:57`, `:114`: a cell with itself, its parent and its neighbours; the overlaps over the candidates summing to the footprint's area to 1e-12 before any normalisation.
- **Used in (S3b):** the coupling of crust pieces and mantle cells, exact in the boundary band and cloud-in-cell normalised inside plates, `@simulation/src/planet/type/Globe.hpp:17`, `@simulation/src/planet/fn/Coupling.hpp:13`. Surface processes, arcs, rifts, plumes and the flow of thick crust book their volumes into the pieces over a cell by the same weights (`forPiecesOver`, `@simulation/src/planet/fn/Coupling.hpp:28`), and the flat view reads cell views by them (`@simulation/src/planet/fn/FlatView.cpp`).
- **Checked by (S3b):** `@simulation/test/GlobeCoupling.cpp` (identity at rest, the sum in every substep of a rigid turn).

### Lauritzen, Nair and Ullrich 2010: CSLAM, overlaps on the cubed sphere
Lauritzen, P. H., Nair, R. D., Ullrich, P. A. (2010). A conservative semi-Lagrangian multi-tracer transport scheme (CSLAM) on the cubed-sphere grid. *Journal of Computational Physics* 229(5), 1401–1424. doi:10.1016/j.jcp.2009.10.036
- **Model:** the overlaps of a moved cell with the grid's cells computed in gnomonic coordinates, where the cubed sphere's cell edges are straight lines.
- **Used in:** `@simulation/src/geometry/fn/Overlap.hpp:8`.

### Sutherland and Hodgman 1974: polygon clipping
Sutherland, I. E., Hodgman, G. W. (1974). Reentrant polygon clipping. *Communications of the ACM* 17(1), 32–42. doi:10.1145/360767.360802
- **Model:** clipping a polygon by each edge of a convex polygon in turn, here in the gnomonic tangent plane.
- **Used in:** `@simulation/src/geometry/fn/Overlap.hpp:10`, `@simulation/src/geometry/fn/Overlap.cpp:60`, `:126`.
- **Checked by:** `@simulation/test/GeometrySphereOverlap.cpp:57`.
- **Used in (S3b):** clipping footprints to the piece or cell a union is measured in, `@simulation/src/geometry/fn/Union.hpp:14`, `@simulation/src/geometry/fn/Union.cpp:87`.
- **Checked by (S3b):** `@simulation/test/GlobeUnion.cpp`.

### Sobel and Feldman 1968: the 3 × 3 gradient
Sobel, I., Feldman, G. (1968). A 3 × 3 isotropic gradient operator for image processing. Talk at the Stanford Artificial Intelligence Project; printed in Duda, R. O., Hart, P. E. (1973), *Pattern Classification and Scene Analysis*, Wiley, 271–272. ISBN 0-471-22361-1.
- **Model:** the direction of a field's gradient from the 1-2-1 weighted differences over a cell's 3 × 3 block, here the elevation over a cube face's grid, so the angle of a shore against the grid lines is measured without the bias of a two-cell difference.
- **Used in (S3b):** the shore-grain metric of `terra globe`, `@simulation/tool/GlobeRocks.cpp:52` (S3-design.md §11 test 12).
