# Credits: wind-blown sand and dust

Part of [Credits](../credits.md); the entry format is explained there.

### Werner 1995: the slab model of dunes
Werner, B. T. (1995). Eolian dunes: computer simulations and attractor interpretation. *Geology* 23(12), 1107. doi:10.1130/0091-7613(1995)023<1107:EDCSAA>2.3.CO;2
- **Model:** sand moves as slabs that hop downwind, settle more on sand than on bare ground, and avalanche past the angle of repose; dunes, slip faces and sand ramps grow from it.
- **Used in:** `@simulation/src/aeolian/fn/Dunes.hpp:8` (in flux form, on a torus); `docs/simulation.md:104`; `docs/handoff/aeolian.md:40`.
- **Approximations:** flux form instead of discrete slabs, at most 500 rounds on the sand the parent tile hands down (`docs/simulation.md:104`).

### Lettau and Lettau 1978: sand flux over a dry bed
Lettau, K., Lettau, H. (1978). Experimental and micrometeorological field studies of dune migration. In: Lettau, H. H., Lettau, K. (eds.), *Exploring the World's Driest Climate*. University of Wisconsin–Madison, Institute for Environmental Studies. (unverified: report number, pages and the coefficient 4.2 not seen in the text)
- **Model:** saltation flux over a dry, flat, sand-covered bed as a function of friction velocity above the threshold, coefficient 4.2.
- **Used in:** `@simulation/src/aeolian/fn/Regime.hpp:13`, `@simulation/src/aeolian/fn/Regime.cpp:15`; the moisture binding of the same flux in `@simulation/src/atmosphere/fn/HazeSources.cpp:81`.

### Bagnold 1941: threshold of grain motion by wind
Bagnold, R. A. (1941). *The Physics of Blown Sand and Desert Dunes*. Methuen, London. (unverified: publisher; reprinted by Springer 1971, doi:10.1007/978-94-009-5682-7)
- **Model:** the friction velocity at which dry grains of a given size start to hop.
- **Used in:** `@simulation/src/aeolian/fn/Regime.hpp:9` (cited by name only).

### Ku et al. 1998 and Lowenstein et al. 1999: deposition in a sinking closed basin
Ku, T.-L., Luo, S., Lowenstein, T. K., et al. (1998). U-series chronology of lacustrine deposits in Death Valley, California. *Quaternary Research* 50(3), 261–275. doi:10.1006/qres.1998.1995

Lowenstein, T. K., Li, J., Brown, C., et al. (1999). 200 k.y. paleoclimate record from Death Valley salt core. *Geology* 27(1), 3. doi:10.1130/0091-7613(1999)027<0003:KYPRFD>2.3.CO;2
- **Model:** about 1 mm/yr of deposition in one of the fastest-sinking closed basins on Earth (Death Valley), the upper bound for how fast a dry basin fills with salt and fine sediment.
- **Used in:** `@simulation/src/aeolian/constant/Wind.hpp:48`.
- **Note:** the 1 mm/yr figure was not checked against the papers' text in this pass.

Dust lifted from dry ground (Fécan et al. 1999) is credited under [atmosphere](atmosphere.md), where the haze owns it.
