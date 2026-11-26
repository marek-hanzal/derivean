# Credits: volcanism

Part of [Credits](../credits.md); the entry format is explained there. Earth's eruption rates the planet is compared with are listed here, beside the models they check.

## Lava, chambers and eruptions

### Hulme 1974: lava as a Bingham body
Hulme, G. (1974). The interpretation of lava flow morphology. *Geophysical Journal of the Royal Astronomical Society* 39(2), 361–383. doi:10.1111/j.1365-246X.1974.tb05460.x
- **Model:** a flow stops where its depth no longer exceeds yield / (ρ g sin α) on a slope (his eq. 14) and spreads on level ground until h² = yield · w (eq. 12); yield strengths of stopped flows from his table 3; the test reads his tables 2 and 3.
- **Used in:** `@simulation/src/volcanic/fn/Volcanic.hpp:42`, `:44`, `:45`; `@simulation/src/volcanic/constant/Rheology.hpp:23`; `docs/handoff/volcanic.md:33`.
- **Checked by:** `@simulation/test/VolcanicPile.cpp:126` against the flows he measured (Mauna Loa 1942, Etna 1966, Hekla 1947, Parícutin 1945).
- **Approximations:** his density 2000 kg/m³ is kept for the yield values (`Rheology.hpp:23`).

### Huppert and Woods 2002: chamber compressibility
Huppert, H. E., Woods, A. W. (2002). The role of volatiles in magma chamber dynamics. *Nature* 420(6915), 493–495. doi:10.1038/nature01211
- **Model:** how much a chamber can take before it erupts, from the compressibility of wall and magma (gas-poor basalt about 2 × 10⁻¹⁰ /Pa, gas-rich andesite 1 × 10⁻⁹ /Pa).
- **Used in:** `@simulation/src/volcanic/fn/Chamber.cpp:15`; `docs/handoff/volcanic.md:76`.

### Roche and Druitt 2001; Geyer, Folch and Martí 2006: when a caldera collapses
Roche, O., Druitt, T. H. (2001). Onset of caldera collapse during ignimbrite eruptions. *Earth and Planetary Science Letters* 191(3–4), 191–202. doi:10.1016/S0012-821X(01)00428-9

Geyer, A., Folch, A., Martí, J. (2006). Relationship between caldera collapse and magma chamber withdrawal: an experimental approach. *Journal of Volcanology and Geothermal Research* 157(4), 375–386. doi:10.1016/j.jvolgeores.2006.05.001
- **Model:** the roof goes down as one block once 0.1 to 0.4 of the chamber has erupted (0.3 used), and only over a chamber wide against its depth.
- **Used in:** `@simulation/src/volcanic/fn/Chamber.cpp:33`.

### Mogi 1958: uplift over a filling chamber
Mogi, K. (1958). Relations between the eruptions of various volcanoes and the deformations of the ground surfaces around them. *Bulletin of the Earthquake Research Institute, University of Tokyo* 36(2), 99–134. doi:10.15083/0000033924
- **Model:** a point source of volume at depth d lifts the ground above it by (1 − ν) ΔV / (π d²).
- **Used in:** `@simulation/src/volcanic/fn/Unrest.cpp:38`, `:39` (cited by name only).

### Box and Muller 1958: normal draws
Box, G. E. P., Muller, M. E. (1958). A note on the generation of random normal deviates. *The Annals of Mathematical Statistics* 29(2), 610–611. doi:10.1214/aoms/1177706645
- **Model:** a normal draw with its whole tail, so the rare great eruption can occur.
- **Used in:** `@simulation/src/volcanic/fn/Chamber.cpp:58`.

### Mastin et al. 2009: eruption column height
Mastin, L. G., Guffanti, M., Servranckx, R., et al. (2009). A multidisciplinary effort to assign realistic source parameters to models of volcanic ash-cloud transport and dispersion during eruptions. *Journal of Volcanology and Geothermal Research* 186(1–2), 10–21. doi:10.1016/j.jvolgeores.2009.01.008
- **Model:** column height against the rate of dense rock through the vent.
- **Used in:** `@simulation/src/volcanic/fn/Outbreak.cpp:26`; `docs/handoff/volcanic.md:23`.

### Settle 1978: plumes over flowing lava
Settle, M. (1978). Volcanic eruption clouds and the thermal power output of explosive eruptions. *Journal of Volcanology and Geothermal Research* 3(3–4), 309–324. doi:10.1016/0377-0273(78)90041-0
- **Model:** a plume rises on the share of its heat that goes into the air.
- **Used in:** `@simulation/src/volcanic/fn/Outbreak.cpp:29`; `docs/handoff/volcanic.md:23`.

### Smith 1960: welding of ash-flow sheets
Smith, R. L. (1960). Zones and zonal variations in welded ash flows. *U.S. Geological Survey Professional Paper* 354-F, 149–159. doi:10.3133/pp354F
- **Model:** a densely welded zone forms only once a sheet is some ten to twenty metres thick.
- **Used in:** `@simulation/src/volcanic/fn/Volcanic.hpp:52`; `docs/handoff/volcanic.md:80`.

## Sizes and rates on Earth

### Newhall and Self 1982: the Volcanic Explosivity Index
Newhall, C. G., Self, S. (1982). The volcanic explosivity index (VEI): an estimate of explosive magnitude for historical volcanism. *Journal of Geophysical Research* 87(C2), 1231–1238. doi:10.1029/JC087iC02p01231
- **Model:** the VEI class of an eruption by its bulk tephra (0 below 10⁴ m³, 1 below 10⁶ m³, …).
- **Used in:** `@simulation/tool/VolcanicSurvey.cpp:29`, `:38`.

### Simkin and Siebert 1994: Volcanoes of the World
Simkin, T., Siebert, L. (1994). *Volcanoes of the World*, 2nd ed. Geoscience Press, Tucson; continued online by the Smithsonian Global Volcanism Program, https://volcano.si.edu/ (unverified: edition and publisher)
- **Reference data:** recurrence of eruptions of a VEI or more on Earth, two weeks for VEI 2 to about 750 years for VEI 7.
- **Used in:** `@simulation/tool/VolcanicSurvey.cpp:30` (the survey compares the planet with these rates).

### Crisp 1984; White, Crisp and Spera 2006: magma output
Crisp, J. A. (1984). Rates of magma emplacement and volcanic output. *Journal of Volcanology and Geothermal Research* 20(3–4), 177–211. doi:10.1016/0377-0273(84)90039-8

White, S. M., Crisp, J. A., Spera, F. J. (2006). Long-term volumetric eruption rates and magma budgets. *Geochemistry, Geophysics, Geosystems* 7(3). doi:10.1029/2005GC001002
- **Reference data:** Earth's eruption rates by setting (arcs, hotspots, rifts) and the intrusive to extrusive ratio; the planet's supply is compared with them.
- **Used in:** `docs/handoff/volcanic.md:24`, `:25`; `docs/handoff/planet.md:59` (comparison only, no constant in code).
- **Note:** `volcanic.md:24` says the first figures were quoted "from memory" before the paper was read.

### Robinson and Eakins 2006: Hawaiian shield volumes
Robinson, J. E., Eakins, B. W. (2006). Calculated volumes of individual shield volcanoes at the young end of the Hawaiian Ridge. *Journal of Volcanology and Geothermal Research* 151(1–3), 309–317. doi:10.1016/j.jvolgeores.2005.07.033
- **Reference data:** Hawaii's supply, 0.095 km³/yr over 6 Myr (0.21 now).
- **Used in:** `docs/handoff/volcanic.md:25` (comparison only).
