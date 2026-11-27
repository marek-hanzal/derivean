# Credits: planet, tectonics and earthquakes

Part of [Credits](../credits.md); the entry format is explained there. The sphere's geometry is under [astronomy and geometry](astronomy-geometry.md); Earth's hypsometry and relief classes the planet is compared with are under [reference data](reference-data.md).

Plate motion, crust growth at arcs and hotspots and the planet's history (`@simulation/src/planet/**`) cite no source today; their constants are tuned to Earth's hypsometry ([gaps](../credits.md#gaps)).

## Isostasy

### Airy 1855: crust floating on the mantle
Airy, G. B. (1855). On the computation of the effect of the attraction of mountain-masses, as disturbing the apparent astronomical latitude of stations in geodetic surveys. *Philosophical Transactions of the Royal Society of London* 145, 101. doi:10.1098/rstl.1855.0003
- **Model:** Airy isostasy: felsic crust (2800 kg/m³) floating on the mantle (3300 kg/m³), its roots below its mountains.
- **Used in:** `@simulation/src/planet/constant/Scale.hpp:18`–`20` (cited by name only).

## Plates on the sphere (S3b)

The history on the cubed sphere (`@simulation/src/planet/{type/Globe.hpp,fn/{Pieces,Rotate,Coupling}.*}`, `@simulation/src/tectonics/{type,fn}/Euler.*`) is additive: nothing in the game reads it until S3e. Design: `.out/lead/handover/S3-design.md` §3–§4.

### McKenzie and Parker 1967; Morgan 1968: plates as rotations about Euler poles
McKenzie, D. P., Parker, R. L. (1967). The North Pacific: an example of tectonics on a sphere. *Nature* 216(5122), 1276–1280. doi:10.1038/2161276a0

Morgan, W. J. (1968). Rises, trenches, great faults, and crustal blocks. *Journal of Geophysical Research* 73(6), 1959–1982. doi:10.1029/JB073i006p01959
- **Model:** a plate is rigid and moves on the sphere as a rotation about its Euler pole; the surface velocity is R (ω × x) and the relative motion across a contact is that of the difference of the two vectors.
- **Used in:** `@simulation/src/tectonics/type/Euler.hpp:10`, `@simulation/src/tectonics/fn/Euler.hpp`, `@simulation/src/planet/fn/Rotate.hpp`.
- **Checked by:** `@simulation/test/GlobeKinematics.cpp` (v = R ω × x, the sign of convergence and shear).

### Argus, Gordon and DeMets 2011: NNR-MORVEL56
Argus, D. F., Gordon, R. G., DeMets, C. (2011). Geologically current motion of 56 plates relative to the no-net-rotation reference frame. *Geochemistry, Geophysics, Geosystems* 12(11), Q11001. doi:10.1029/2011GC003751
- **Model:** the no-net-rotation frame as the mantle's frame; plate speeds: RMS of the whole lithosphere 43.9 mm/yr, the Pacific 65.5 mm/yr (0.651°/Myr) and Eurasia 22.8 mm/yr (0.223°/Myr) as the anchors of the fastest oceanic and slowest continental plate.
- **Used in:** `@simulation/src/tectonics/type/Euler.hpp:24`, `@simulation/src/tectonics/fn/Euler.hpp:19`, `:41`, `@simulation/src/tectonics/fn/Euler.cpp:17`.
- **Checked by:** `@simulation/test/GlobeKinematics.cpp` (no net rotation after damping); the RMS against 43.9 mm/yr in `terra globe` (`docs/handoff/planet-globe.md`).

### Forsyth and Uyeda 1975: what drives plates
Forsyth, D., Uyeda, S. (1975). On the relative importance of the driving forces of plate motion. *Geophysical Journal of the Royal Astronomical Society* 43(1), 163–200. doi:10.1111/j.1365-246X.1975.tb00631.x
- **Model:** slab pull is about an order of magnitude stronger than the other forces and basal drag is stronger under continents, so plates with slabs move fast and continental plates slowly; a plate's speed falls with its continental share.
- **Used in:** `@simulation/src/tectonics/type/Euler.hpp:25`, `@simulation/src/tectonics/fn/Euler.cpp` (`targetSpeed`).

### Stern 2002: which plate sinks at a trench
Stern, R. J. (2002). Subduction zones. *Reviews of Geophysics* 40(4), 1012. doi:10.1029/2001RG000108
- **Model:** oceanic lithosphere sinks because it grows denser as it ages, so at a trench between two oceans the older plate goes down; a trench keeps its polarity, which turns only in a reorganisation such as an arc meeting a continent.
- **Used in:** `@simulation/src/planet/fn/Boundary.hpp` (`sinksUnder`), `@simulation/src/planet/fn/Rotate.cpp` (each plate's mean ocean age as its epoch starts).

### Bird 2003: the plates of today's Earth
Bird, P. (2003). An updated digital model of plate boundaries. *Geochemistry, Geophysics, Geosystems* 4(3), 1027. doi:10.1029/2001GC000252
- **Reference data:** 52 plates, 14 large and 38 small; each plate is one region, the small ones plates of their own; the list is incomplete below the Shetland plate, 72 240 km².
- **Used in:** `@simulation/test/GlobeCohesion.hpp` (a plate's cells outside its largest region), the cohesion gate in `docs/handoff/planet-globe.md` (stage 2b).

### White, McKenzie and O'Nions 1992: thickness of oceanic crust
White, R. S., McKenzie, D., O'Nions, R. K. (1992). Oceanic crustal thickness from seismic measurements and rare earth element inversions. *Journal of Geophysical Research: Solid Earth* 97(B13), 19683–19715. doi:10.1029/92JB01749
- **Reference data:** normal oceanic crust is 7.1 ± 0.8 km thick; new ocean floor takes 7 km of basalt from the interior and a trench returns it.
- **Used in:** `@simulation/src/planet/constant/Globe.hpp:11`.

## Lithosphere (planned, planet slice S3)

### Kohlstedt, Evans and Mackwell 1995: strength of the lithosphere
Kohlstedt, D. L., Evans, B., Mackwell, S. J. (1995). Strength of the lithosphere: constraints imposed by laboratory experiments. *Journal of Geophysical Research* 100(B9), 17587–17602. doi:10.1029/95JB01460
- **Model:** the brittle–ductile transition depends on temperature, pressure, strain rate and pore conditions; the assumptions of any crust or mantle material model are to be documented against it.
- **Used in:** planned, `.out/lead/handover/PLANET.md:241`.

## Earthquakes (planned on the sphere)

The earthquake plan is in `docs/handoff/README.md:101`–`103` and `.out/review-mailbox/from-claude/DREV-008-02.txt:14`–`25`. Nothing is written yet.

### Gutenberg and Richter 1944: sizes of quakes
Gutenberg, B., Richter, C. F. (1944). Frequency of earthquakes in California. *Bulletin of the Seismological Society of America* 34(4), 185–188. doi:10.1785/BSSA0340040185
- **Model:** the number of quakes falls tenfold per magnitude (log N = a − b M).
- **Used in:** planned, `docs/handoff/README.md:101`.

### Wells and Coppersmith 1994: rupture length and slip
Wells, D. L., Coppersmith, K. J. (1994). New empirical relationships among magnitude, rupture length, rupture width, rupture area, and surface displacement. *Bulletin of the Seismological Society of America* 84(4), 974–1002. doi:10.1785/BSSA0840040974
- **Model:** rupture length, width, area and displacement against magnitude by fault type.
- **Used in:** planned, `docs/handoff/README.md:101`.

### Okada 1985: ground displacement of a fault
Okada, Y. (1985). Surface deformation due to shear and tensile faults in a half-space. *Bulletin of the Seismological Society of America* 75(4), 1135–1154. doi:10.1785/BSSA0750041135
- **Model:** the closed-form surface displacement of a rectangular fault in an elastic half-space: scarps, uplifted terraces, subsided coasts.
- **Used in:** planned, `docs/handoff/README.md:102`.

### Newmark 1965; Keefer 1984: landslides from shaking
Newmark, N. M. (1965). Effects of earthquakes on dams and embankments. *Géotechnique* 15(2), 139–160. doi:10.1680/geot.1965.15.2.139

Keefer, D. K. (1984). Landslides caused by earthquakes. *Geological Society of America Bulletin* 95(4), 406. doi:10.1130/0016-7606(1984)95<406:LCBE>2.0.CO;2
- **Model:** a slope slides by the displacement of a rigid block shaken past its yield acceleration (Newmark); the area and volume of landslides against magnitude (Keefer).
- **Used in:** planned, `docs/handoff/README.md:103`.
