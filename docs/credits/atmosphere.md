# Credits: atmosphere, haze, clouds and snow

Part of [Credits](../credits.md); the entry format is explained there. The sky's light as drawn is under [rendering](rendering.md).

## The stepped atmosphere

### Arakawa 1966: the energy-conserving Jacobian
Arakawa, A. (1966). Computational design for long-term numerical integration of the equations of fluid motion: two-dimensional incompressible flow. Part I. *Journal of Computational Physics* 1(1), 119–143. doi:10.1016/0021-9991(66)90015-5
- **Model:** advection of vorticity by a Jacobian that keeps energy and enstrophy from piling up at the grid scale.
- **Used in:** `@simulation/src/atmosphere/fn/Circulation.cpp:62` (cited by name only).

### Robert 1966, Asselin 1972 and Williams 2009: the leapfrog filter
Robert, A. J. (1966). The integration of a low order spectral form of the primitive meteorological equations. *Journal of the Meteorological Society of Japan, Ser. II* 44(5), 237–245. doi:10.2151/jmsj1965.44.5_237

Asselin, R. (1972). Frequency filter for time integrations. *Monthly Weather Review* 100(6), 487–490. doi:10.1175/1520-0493(1972)100<0487:FFFTI>2.3.CO;2

Williams, P. D. (2009). A proposed modification to the Robert–Asselin time filter. *Monthly Weather Review* 137(8), 2538–2546. doi:10.1175/2009MWR2724.1
- **Model:** the Robert–Asselin–Williams filter damping the computational mode of the leapfrog step, strength 0.1.
- **Used in:** `@simulation/src/atmosphere/fn/Circulation.cpp:15` (cited by name only).

### Alduchov and Eskridge 1996: the Magnus formula
Alduchov, O. A., Eskridge, R. E. (1996). Improved Magnus form approximation of saturation vapor pressure. *Journal of Applied Meteorology* 35(4), 601–609. doi:10.1175/1520-0450(1996)035<0601:IMFAOS>2.0.CO;2
- **Model:** saturation vapour density by the Magnus form, tabulated at compile time.
- **Used in:** `@simulation/src/climate/fn/Moisture.hpp:10`, `:35`. The code names only "the Magnus formula"; its coefficients (610.94 Pa, 17.625, 243.04 °C) are this paper's.

### WMO International Cloud Atlas: the ten genera
World Meteorological Organization. *International Cloud Atlas: Manual on the Observation of Clouds and Other Meteors* (WMO-No. 407). WMO, Geneva; online edition 2017 onwards. https://cloudatlas.wmo.int/ (unverified: edition year)
- **Model:** the ten cloud genera and their typical base and top heights.
- **Used in:** `@simulation/src/atmosphere/type/Cloud.hpp:12`, `@game/src/clouds/fn/findWeather.ts:5`, `@game/contract/GameBridge.ts:267`; `docs/handoff/brief/clouds.md:21`.
- **Checked by:** `@simulation/test/Clouds.cpp:70`, one scene per genus.

## Light through clean air

### Bucholtz 1995: Rayleigh extinction of clean air
Bucholtz, A. (1995). Rayleigh-scattering calculations for the terrestrial atmosphere. *Applied Optics* 34(15), 2765. doi:10.1364/AO.34.002765
- **Model:** 1.16 × 10⁻⁵ /m at 550 nm at sea level.
- **Used in:** `@simulation/src/atmosphere/constant/Light.hpp:13`; through `@napi/src/addon.cpp:131` into the renderer.
- **Checked by:** `@simulation/test/Haze.cpp:148`, clean air sees 337 km.
- **Approximations:** the dispersion of the refractive index (about +0.05 on the exponent) is left out (`Light.hpp:15`).

### Rayleigh 1871: the inverse fourth power
Strutt, J. W. (Lord Rayleigh) (1871). On the light from the sky, its polarization and colour. *Philosophical Magazine*, 4th series, 41. Reprinted in *Scientific Papers* vol. 1, Cambridge University Press, 87–103, doi:10.1017/CBO9780511703966.009. (unverified: Philosophical Magazine pages)
- **Model:** scattering by clean air as λ⁻⁴.
- **Used in:** `@simulation/src/atmosphere/constant/Light.hpp:14`.

### Mie 1908: scattering by particles
Mie, G. (1908). Beiträge zur Optik trüber Medien, speziell kolloidaler Metallösungen. *Annalen der Physik* 330(3), 377–445. doi:10.1002/andp.19083300302
- **Model:** mass extinction, spectrum and single-scattering albedo of each aerosol kind, computed offline and stored as constants.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:11`, `:14`, `:16`.
- **Note:** the Mie code or tables the constants were computed with are not recorded ([gaps](../credits.md#gaps)).

## Haze: optics, growth and removal

### Petters and Kreidenweis 2007: κ hygroscopicity
Petters, M. D., Kreidenweis, S. M. (2007). A single parameter representation of hygroscopic growth and cloud condensation nucleus activity. *Atmospheric Chemistry and Physics* 7(8), 1961–1971. doi:10.5194/acp-7-1961-2007
- **Model:** a particle at relative humidity h holds 1 + κ·h/(1 − h) times its dry volume.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:12`; `docs/handoff/atmosphere.md:19`.

### Köhler 1936: droplet activation
Köhler, H. (1936). The nucleus in and the growth of hygroscopic droplets. *Transactions of the Faraday Society* 32, 1152–1161. doi:10.1039/TF9363201152
- **Model:** every haze kind is large enough to activate at a cloud's supersaturation (κ-Köhler), so it rains out with the low band's water.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:44` (cited by name only).

### Koehler et al. 2009: κ of mineral dust
Koehler, K. A., Kreidenweis, S. M., DeMott, P. J., et al. (2009). Hygroscopicity and cloud droplet activation of mineral dust aerosol. *Geophysical Research Letters* 36(8). doi:10.1029/2009GL037348
- **Model:** κ = 0.03 for dust.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:32`.

### Zieger et al. 2017: κ of sea salt
Zieger, P., Väisänen, O., Corbin, J. C., et al. (2017). Revising the hygroscopicity of inorganic sea salt particles. *Nature Communications* 8, 15883. doi:10.1038/ncomms15883
- **Model:** κ = 1.1 for sea salt.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:34`.

### Dubovik et al. 2002: absorption of Saharan dust
Dubovik, O., Holben, B., Eck, T. F., et al. (2002). Variability of absorption and optical properties of key aerosol types observed in worldwide locations. *Journal of the Atmospheric Sciences* 59(3), 590–608. doi:10.1175/1520-0469(2002)059<0590:VOAAOP>2.0.CO;2
- **Model:** refractive index of dust, 1.53 + 0.0010i to 0.0025i by colour.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:19`; `docs/handoff/atmosphere.md:19`.

### Stokes 1851: settling
Stokes, G. G. (1851). On the effect of the internal friction of fluids on the motion of pendulums. *Transactions of the Cambridge Philosophical Society* 9(2), 8–106.
- **Model:** fall speed of a swollen particle in air.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:47` (cited by name only).

### Wesely et al. 1985: dry deposition
Wesely, M. L., Cook, D. R., Hart, R. L., Speer, R. E. (1985). Measurements and parameterization of particulate sulfur dry deposition over grass. *Journal of Geophysical Research* 90(D1), 2131–2143. doi:10.1029/JD090iD01p02131
- **Model:** deposition velocity of 0.002 u* for accumulation-mode sulphate.
- **Used in:** `@simulation/src/atmosphere/fn/Haze.cpp:48`.

## Haze: sources

### Fécan, Marticorena and Bergametti 1999: moisture and the dust threshold
Fécan, F., Marticorena, B., Bergametti, G. (1999). Parametrization of the increase of the aeolian erosion threshold wind friction velocity due to soil moisture for arid and semi-arid areas. *Annales Geophysicae* 17(1), 149–157. doi:10.1007/s00585-999-0149-7
- **Model:** the threshold friction velocity rises with the gravimetric moisture of a sand without clay.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:19`, `:104`; `docs/handoff/atmosphere.md:19`.

### Gong 2003: sea spray
Gong, S. L. (2003). A parameterization of sea-salt aerosol source function for sub- and super-micron particles. *Global Biogeochemical Cycles* 17(4). doi:10.1029/2003GB002079
- **Model:** spray flux integrated over wet radii up to 4 µm at 80 % humidity, as U^3.41.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:24`, `@simulation/src/atmosphere/fn/Haze.cpp:34`.

### Nightingale et al. 2000 and Saltzman et al. 1993: dimethyl sulphide from the sea
Nightingale, P. D., Malin, G., Law, C. S., et al. (2000). In situ evaluation of air-sea gas exchange parameterizations using novel conservative and volatile tracers. *Global Biogeochemical Cycles* 14(1), 373–387. doi:10.1029/1999GB900091

Saltzman, E. S., King, D. B., Holmen, K., Leck, C. (1993). Experimental determination of the diffusion coefficient of dimethylsulfide in water. *Journal of Geophysical Research* 98(C9), 16481–16486. doi:10.1029/93JC01858
- **Model:** the transfer velocity against wind, scaled to the Schmidt number of DMS in sea water.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:30`, `:31`, `:110`.

### Lana et al. 2011: DMS in the sea
Lana, A., Bell, T. G., Simó, R., et al. (2011). An updated climatology of surface dimethlysulfide concentrations and emission fluxes in the global ocean. *Global Biogeochemical Cycles* 25(1). doi:10.1029/2010GB003850 (the title's spelling is the journal's)
- **Model:** Earth's mean surface concentration, 2.3 nM, until the sea has life of its own.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:32`.

### Chin et al. 1996: sulphur to sulphate
Chin, M., Jacob, D. J., Gardner, G. M., et al. (1996). A global three-dimensional model of tropospheric sulfate. *Journal of Geophysical Research* 101(D13), 18667–18690. doi:10.1029/96JD01221
- **Model:** about 0.4 of DMS sulphur ends as sulphate aerosol.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:34`.

### Guenther et al. 1993, 1995 and 2006: plant emissions
Guenther, A. B., Zimmerman, P. R., Harley, P. C., et al. (1993). Isoprene and monoterpene emission rate variability: model evaluations and sensitivity analyses. *Journal of Geophysical Research* 98(D7), 12609–12617. doi:10.1029/93JD00527

Guenther, A., Hewitt, C. N., Erickson, D., et al. (1995). A global model of natural volatile organic compound emissions. *Journal of Geophysical Research* 100(D5), 8873–8892. doi:10.1029/94JD02950

Guenther, A., Karl, T., Harley, P., et al. (2006). Estimates of global terrestrial isoprene emissions using MEGAN (Model of Emissions of Gases and Aerosols from Nature). *Atmospheric Chemistry and Physics* 6(11), 3181–3210. doi:10.5194/acp-6-3181-2006
- **Model:** isoprene against leaf temperature and light (1993), monoterpenes always (1995), canopy emission factors of a mixed cover at 30 °C and full light (MEGAN, 2006).
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:40`, `:41`, `:118`.

### Kroll et al. 2006, Griffin et al. 1999 and Hallquist et al. 2009: secondary organic aerosol
Kroll, J. H., Ng, N. L., Murphy, S. M., et al. (2006). Secondary organic aerosol formation from isoprene photooxidation. *Environmental Science & Technology* 40(6), 1869–1877. doi:10.1021/es0524301

Griffin, R. J., Cocker, D. R., Flagan, R. C., Seinfeld, J. H. (1999). Organic aerosol formation from the oxidation of biogenic hydrocarbons. *Journal of Geophysical Research* 104(D3), 3555–3567. doi:10.1029/1998JD100049

Hallquist, M., Wenger, J. C., Baltensperger, U., et al. (2009). The formation, properties and impact of secondary organic aerosol: current and emerging issues. *Atmospheric Chemistry and Physics* 9(14), 5155–5236. doi:10.5194/acp-9-5155-2009
- **Model:** yields of about 3 % for isoprene and 10 % for monoterpenes.
- **Used in:** `@simulation/src/atmosphere/fn/HazeSources.cpp:42`, `:43`.

## Snow (approved, not yet written)

### Sturm et al. 2010: bulk density of snow
Sturm, M., Taras, B., Liston, G. E., et al. (2010). Estimating snow water equivalent using snow depth data and climate classes. *Journal of Hydrometeorology* 11(6), 1380–1394. doi:10.1175/2010JHM1202.1
- **Model:** snow density by depth, day of the season and snow class (their Table 4); depth per ground cell from SWE.
- **Used in:** planned, `docs/handoff/engine.md:54`, `:131`, `:150`, `:157`.
- **Validation planned:** against the paper's curves and a slope case (`docs/handoff/engine.md:157`).

### Sturm, Holmgren and Liston 1995: snow classes
Sturm, M., Holmgren, J., Liston, G. E. (1995). A seasonal snow cover classification system for local to global applications. *Journal of Climate* 8(5), 1261–1283. doi:10.1175/1520-0442(1995)008<1261:ASSCCS>2.0.CO;2
- **Model:** the snow class from the climate.
- **Used in:** planned, `docs/handoff/engine.md:131`.

Firn densification (Herron and Langway 1980) is under [ice](ice.md).
