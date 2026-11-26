# Credits: planet, tectonics and earthquakes

Part of [Credits](../credits.md); the entry format is explained there. The sphere's geometry is under [astronomy and geometry](astronomy-geometry.md); Earth's hypsometry and relief classes the planet is compared with are under [reference data](reference-data.md).

Plate motion, crust growth at arcs and hotspots and the planet's history (`@simulation/src/planet/**`) cite no source today; their constants are tuned to Earth's hypsometry ([gaps](../credits.md#gaps)).

## Isostasy

### Airy 1855: crust floating on the mantle
Airy, G. B. (1855). On the computation of the effect of the attraction of mountain-masses, as disturbing the apparent astronomical latitude of stations in geodetic surveys. *Philosophical Transactions of the Royal Society of London* 145, 101. doi:10.1098/rstl.1855.0003
- **Model:** Airy isostasy: felsic crust (2800 kg/m³) floating on the mantle (3300 kg/m³), its roots below its mountains.
- **Used in:** `@simulation/src/planet/constant/Scale.hpp:18`–`20` (cited by name only).

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
