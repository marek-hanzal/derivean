# Credits: hydrology and groundwater

Part of [Credits](../credits.md); the entry format is explained there. River sediment transport at a mouth is under [coasts](coasts.md); measured lakes the evaporation is checked against are under [reference data](reference-data.md).

## Evaporation and the water balance

The shared evaporation (`hydrology/fn/Evaporation.cpp`) is held in a patch and not yet in the tree: `.out/groundwater/evap/patched/@simulation/src/hydrology/fn/Evaporation.cpp`, described in `docs/handoff/groundwater.md:124`. Line numbers in this section are in that patched file.

### Allen et al. 1998 (FAO-56): net radiation and vapour pressure
Allen, R. G., Pereira, L. S., Raes, D., Smith, M. (1998). *Crop evapotranspiration: guidelines for computing crop water requirements*. FAO Irrigation and Drainage Paper 56. FAO, Rome. ISBN 92-5-104219-5. https://www.fao.org/3/X0490E/X0490E00.htm
- **Model:** net radiation from top-of-atmosphere sunshine, the Ångström–Prescott fraction and the net longwave (eq. 39); saturation vapour pressure (eq. 11); the psychrometric constant with height (eq. 7, probed, not written).
- **Used in:** `Evaporation.cpp:10`, `:39` (held); `docs/handoff/groundwater.md:50`, `:124`.
- **Approximations:** γ at sea level for every caller. The hand-over says a lake at 3.8 km then evaporates about 22 % short (`docs/handoff/groundwater.md:124`), the patched code says a lake at 3 km about a third short (`Evaporation.cpp:24`); one of them is wrong ([gaps](../credits.md#gaps)).

### North 1975: annual sunshine by latitude
North, G. R. (1975). Analytical solution to a simple climate model with diffusive heat transport. *Journal of the Atmospheric Sciences* 32(7), 1301–1307. doi:10.1175/1520-0469(1975)032<1301:ASTASC>2.0.CO;2
- **Model:** annual-mean insolation at the top of the atmosphere as the second Legendre term in the sine of latitude, coefficient 0.482.
- **Used in:** `Evaporation.cpp:11` (held).
- **Note:** North's "Theory of energy-balance climate models" (J. Atmos. Sci. 32(11), 1975) 32(11), 2033–2043, carries the same form; the code does not say which of the two (unverified which).

### Ångström 1924 and Prescott 1940: sunshine reaching the ground
Ångström, A. (1924). Solar and terrestrial radiation. *Quarterly Journal of the Royal Meteorological Society* 50(210), 121–126. doi:10.1002/qj.49705021008

Prescott, J. A. (1940). Evaporation from a water surface in relation to solar radiation. *Transactions of the Royal Society of South Australia* 64. (unverified: issue and pages)
- **Model:** the share of top-of-atmosphere radiation reaching the ground as a + b × sunshine fraction (a 0.25, b 0.5).
- **Used in:** `Evaporation.cpp:11`, `:18` (held).

### Priestley and Taylor 1972: evaporation of wet ground and open water
Priestley, C. H. B., Taylor, R. J. (1972). On the assessment of surface heat flux and evaporation using large-scale parameters. *Monthly Weather Review* 100(2), 81–92. doi:10.1175/1520-0493(1972)100<0081:OTAOSH>2.3.CO;2
- **Model:** equilibrium evaporation times α = 1.26 for wet land and large lakes.
- **Used in:** `Evaporation.cpp:25`, `:72` (held); `docs/handoff/groundwater.md:124`.

### Fu 1981 and Zhang et al. 2004: land evaporation against rain and demand
Fu, B. P. (1981). On the calculation of the evaporation from land surface. *Scientia Atmospherica Sinica* 5(1), 23 (in Chinese). (unverified: end page and English title)

Zhang, L., Hickel, K., Dawes, W. R., et al. (2004). A rational function approach for estimating mean annual evapotranspiration. *Water Resources Research* 40(2). doi:10.1029/2003WR002710
- **Model:** E = P + PET − (P^ω + PET^ω)^(1/ω), tending to P in dry and to PET in wet country; ω = 2.6 from Zhang's global fit.
- **Used in:** `Evaporation.cpp:28`, `:75` (held); `docs/handoff/groundwater.md:56`, `:124`.

### Bouchet 1963 and Morton 1983: the complementary relationship
Bouchet, R. J. (1963). Évapotranspiration réelle et potentielle, signification climatique. *IAHS Publication* 62, 134–142. (pages from citing papers; original not seen)

Morton, F. I. (1983). Operational estimates of areal evapotranspiration and their significance to the science and practice of hydrology. *Journal of Hydrology* 66(1–4), 1–76. doi:10.1016/0022-1694(83)90177-4
- **Model:** a lake in dry country takes part of the gap between the wet rate and the land's, because the air the dry land heats and dries passes over it; share 0.2, calibrated on Lake Mead and Lake Nasser.
- **Used in:** `Evaporation.cpp:31` (held); `docs/handoff/groundwater.md:124`.
- **Approximations:** asymmetric share instead of the symmetric relationship (k = 1 gives 4.1 m/yr at Nasser, rejected).

### Budyko 1974: the mean water balance
Budyko, M. I. (1974). *Climate and Life*. International Geophysics Series 18. Academic Press, New York. ISBN 0-12-139450-6.
- **Model:** how much rain is left after evaporation from the ratio of demand to rain.
- **Used in:** `@simulation/src/landscape/fn/Forcing.hpp:51` (cited by name; the closed form there is the code's own); `docs/handoff/groundwater.md:123`.

## Flow, routing and ground water

### Manning 1891: friction of open-channel flow
Manning, R. (1891). On the flow of water in open channels and pipes. *Transactions of the Institution of Civil Engineers of Ireland* 20, 161–207.
- **Model:** depth-averaged flow speed against slope and bed roughness; n = 0.035 for a natural bed.
- **Used in:** `@simulation/src/hydrology/constant/Water.hpp:9`, `@simulation/src/hydrology/fn/Surge.cpp:21`.

### Darcy 1856: seepage through porous ground
Darcy, H. (1856). *Les fontaines publiques de la ville de Dijon*. Victor Dalmont, Paris. (Note D, the sand-column experiments)
- **Model:** seepage through a gravel barrier proportional to head gradient; named as a secondary, not modelled, outflow.
- **Used in:** `docs/handoff/coast.md:186` (reasoning only, no code).

### Torricelli 1644; Kirchhoff 1869; Lienhard and Lienhard 1984: flow through a strait's throat
Torricelli, E. (1644). *Opera geometrica*. Florentiae: typis Amatoris Massae & Laurentii de Landis. https://archive.org/details/operageometrica00torr (the section *De motu gravium naturaliter descendentium* and its appendix on the motion of water; the page of the efflux statement not checked)

Kirchhoff, G. (1869). Zur Theorie freier Flüssigkeitsstrahlen. *Journal für die reine und angewandte Mathematik* 70, 289–298. doi:10.1515/crll.1869.70.289 (the value 0.611 is the standard result of this theory, not read from the paper)

Lienhard (V), J. H., Lienhard (IV), J. H. (1984). Velocity coefficients for free jets from sharp-edged orifices. *Journal of Fluids Engineering* 106(1), 13–17. doi:10.1115/1.3242391
- **Model:** water leaves an opening at √(2gH) under the head H over it (Torricelli); the jet of a sharp-edged opening contracts to π/(π+2) ≈ 0.61 of its area (Kirchhoff, free streamlines); the velocity coefficient of such jets is close to 1 (Lienhard and Lienhard), so the discharge coefficient is the contraction, C = 0.6. A strait passes Q = C·A·√(2gH); with the lower surface over the bed the same law runs submerged under the difference of the two surfaces.
- **Used in:** `@simulation/src/hydrology/fn/Strait.cpp:55`, `@simulation/src/hydrology/constant/Water.hpp:10`.
- **Checked by:** `testVerticalWalls` in `@simulation/test/HydrologySea.cpp` (free and submerged outflow against the closed form, 1e-9).
- **Approximations:** Kirchhoff's value is for a two-dimensional slot; measured round sharp-edged orifices give 0.60–0.62. A dug channel is not an orifice: its friction and length are not modelled, and the submerged case uses the same coefficient.

### Garcia-Castellanos et al. 2009: a strait floods a basin below sea level
Garcia-Castellanos, D., Estrada, F., Jiménez-Munt, I., Gorini, C., Fernàndez, M., Vergés, J., De Vicente, R. (2009). Catastrophic flood of the Mediterranean after the Messinian salinity crisis. *Nature* 462(7274), 778–781. doi:10.1038/nature08555
- **Reference data:** the Zanclean flood, the Atlantic refilling a drawn-down Mediterranean through the Strait of Gibraltar: Earth's case of the planet's acceptance scenario, a strait into a dry basin below sea level.
- **Used in:** `docs/handoff/interventions.md` (the acceptance scenario); the design review that asked for it (`.out/review-mailbox/from-codex/DREV-CLAUDE-003-01.txt`, V8).

### Fournier 1977: silica held by water against quartz
Fournier, R. O. (1977). Chemical geothermometers and mixing models for geothermal systems. *Geothermics* 5(1–4), 41–50. doi:10.1016/0375-6505(77)90007-4
- **Model:** quartz solubility log₁₀ C = 5.19 − 1309 / T, the silica a spring brings up from hot rock.
- **Used in:** `@simulation/src/groundwater/fn/Chemistry.hpp:13`.

## Daily flow over the year

The flow-duration model is held in the coast stream's mirror and not yet in the tree: `.out/coast/mirror/src/hydrology/fn/FlowDuration.{hpp,cpp}`, described in `docs/handoff/coast.md:24`. Line numbers in this section are in those files.

### Wolman and Miller 1960: effective discharge
Wolman, M. G., Miller, J. P. (1960). Magnitude and frequency of forces in geomorphic processes. *The Journal of Geology* 68(1), 54–74. doi:10.1086/626637
- **Model:** what a year of flows does to sediment is not what the mean flow would do, so transport is integrated over the year's flows.
- **Used in:** `FlowDuration.hpp:10` (held); `docs/handoff/coast.md:25`.

### Vogel and Fennessey 1994: lognormal daily flow
Vogel, R. M., Fennessey, N. M. (1994). Flow-duration curves. I: New interpretation and confidence intervals. *Journal of Water Resources Planning and Management* 120(4), 485–504. doi:10.1061/(ASCE)0733-9496(1994)120:4(485)
- **Model:** while a stream runs, its daily flow is lognormal.
- **Used in:** `FlowDuration.hpp:12` (held); `docs/handoff/coast.md:24`.

### Rodriguez-Iturbe et al. 1999; Laio et al. 2001: storms as marked Poisson rain
Rodriguez-Iturbe, I., Porporato, A., Ridolfi, L., et al. (1999). Probabilistic modelling of water balance at a point: the role of climate, soil and vegetation. *Proceedings of the Royal Society A* 455(1990), 3789–3805. doi:10.1098/rspa.1999.0477

Laio, F., Porporato, A., Ridolfi, L., Rodriguez-Iturbe, I. (2001). Plants in water-controlled ecosystems: active role in hydrologic processes and response to water stress. II. Probabilistic soil moisture dynamics. *Advances in Water Resources* 24(7), 707–723. doi:10.1016/S0309-1708(01)00005-7
- **Model:** storms at random times with exponential depths, mean 1 cm (0.5–2 cm measured).
- **Used in:** `FlowDuration.hpp:14`, `FlowDuration.cpp:11` (held).

### Botter et al. 2007: shot-noise streamflow
Botter, G., Porporato, A., Rodriguez-Iturbe, I., Rinaldo, A. (2007). Basin-scale soil moisture dynamics and the probabilistic characterization of carrier hydrologic flows: slow, leaching-prone components of the hydrologic response. *Water Resources Research* 43(2). doi:10.1029/2006WR005043
- **Model:** a basin as a linear reservoir of residence τ fed by λ storms a day gives CV² = 1/(λτ), joined to the daily-rain limit 2/λ.
- **Used in:** `FlowDuration.hpp:16` (held); `docs/handoff/coast.md:24`.
- **Note:** the base-flow paper of the same group (Botter et al. 2007, WRR 43(6), doi:10.1029/2006WR005397) may be the one meant; the code does not say (unverified which).

### Nash 1957; Snyder 1938; Hack 1957: the basin's lag
Nash, J. E. (1957). The form of the instantaneous unit hydrograph. *IASH Publication* 45(3), 114–121.

Snyder, F. F. (1938). Synthetic unit-graphs. *Transactions, American Geophysical Union* 19(1), 447–454. doi:10.1029/TR019i001p00447

Hack, J. T. (1957). *Studies of Longitudinal Stream Profiles in Virginia and Maryland*. U.S. Geological Survey Professional Paper 294-B. doi:10.3133/pp294B
- **Model:** the residence time of a single linear reservoir (Nash) as Snyder's lag t = Ct·(L·Lc)^0.3 h with Ct = 2, and the main stream's length from basin area by Hack's law L = 1.4·A^0.6 (miles, square miles).
- **Used in:** `FlowDuration.hpp:25`, `:26`, `:28`, `FlowDuration.cpp:19`, `:21` (held). Hack's law in the tree: see [landscape](landscape.md).
- **Approximations:** no slow ground-water store between storms (`FlowDuration.hpp:29`).

### Leopold, Wolman and Miller 1964; Dury 1973: the bankfull flood
Leopold, L. B., Wolman, M. G., Miller, J. P. (1964). *Fluvial Processes in Geomorphology*. W. H. Freeman, San Francisco.

Dury, G. H. (1973). Magnitude-frequency analysis and channel morphometry. In: Morisawa, M. (ed.), *Fluvial Geomorphology*, 91–121. Publications in Geomorphology, SUNY Binghamton. (unverified: 1973 imprint)
- **Model:** the bankfull flood is the 1.5-year flood, the largest flood of the year exceeded two years in three.
- **Used in:** `FlowDuration.hpp:40` (held); `docs/handoff/coast.md:24`.
