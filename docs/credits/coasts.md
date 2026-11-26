# Credits: coasts, waves and river mouths

Part of [Credits](../credits.md); the entry format is explained there.

## Waves

### Sverdrup and Munk 1947; Bretschneider 1952: deep-water wave growth
Sverdrup, H. U., Munk, W. H. (1947). *Wind, Sea, and Swell: Theory of Relations for Forecasting*. U.S. Navy Hydrographic Office, H.O. Pub. 601. doi:10.5962/bhl.title.38751

Bretschneider, C. L. (1952). The generation and decay of wind waves in deep water. *Transactions, American Geophysical Union* 33(3), 381–389. doi:10.1029/TR033i003p00381
- **Model:** fetch-limited growth g·H/U² = 0.0016·(g·F/U²)^½ (the Sverdrup–Munk–Bretschneider curve).
- **Used in:** `@simulation/src/coast/constant/Waves.hpp:17`.

### U.S. Army Corps of Engineers 1984: Shore Protection Manual
Coastal Engineering Research Center (1984). *Shore Protection Manual*, 4th ed., 2 vols. U.S. Army Engineer Waterways Experiment Station, Vicksburg, MS. U.S. Government Printing Office, Washington, DC.
- **Model:** fetch-limited growth over water of any depth (eq. 3-39); the CERC longshore transport formula with K = 0.39 for the significant height.
- **Used in:** `@simulation/src/coast/fn/Waves.cpp:13`, `@simulation/src/coast/fn/Drift.cpp:24`; `docs/handoff/coast.md:15`.
- **Checked by:** `@simulation/test/Coast.cpp:83`, wave growth against the manual's forecast.

### Fenton and McKee 1990: wavelength in any depth
Fenton, J. D., McKee, W. D. (1990). On calculating the lengths of water waves. *Coastal Engineering* 14(6), 499–513. doi:10.1016/0378-3839(90)90032-R
- **Model:** explicit kh from the deep-water k₀h, so energy travels at n·c.
- **Used in:** `@simulation/src/coast/fn/Waves.cpp:30`.

### Sallenger 2000: overwash
Sallenger, A. H., Jr. (2000). Storm impact scale for barrier islands. *Journal of Coastal Research* 16(3), 890–895. https://pubs.usgs.gov/publication/70022487
- **Model:** a barrier is overwashed where run-up exceeds its crest.
- **Used in:** `@simulation/src/coast/fn/Waves.cpp:79`.

## Cliffs, beaches and banks

### Sunamura 1977: wave force against rock
Sunamura, T. (1977). A relationship between wave-induced cliff erosion and erosive force of waves. *The Journal of Geology* 85(5), 613–618. doi:10.1086/628340
- **Model:** a cliff foot is cut once the breaking-wave force ρgH exceeds a share (about 2 × 10⁻⁴) of the rock's compressive strength, then as (H − Hc)/Hc².
- **Used in:** `@simulation/src/coast/constant/Waves.hpp:40`.

### Sunamura 1984: beach-face slope
Sunamura, T. (1984). Quantitative predictions of beach-face slopes. *Geological Society of America Bulletin* 95(2), 242. doi:10.1130/0016-7606(1984)95<242:QPOBS>2.0.CO;2
- **Model:** tanβ = 0.12·(H/(√(gD)·T))^−½ from grain size and breaker height.
- **Used in:** `@simulation/src/coast/fn/Beach.cpp:11`; `docs/simulation.md:101`; `docs/handoff/coast.md:17`, `:133`.

### Dean 1991: the equilibrium shoreface
Dean, R. G. (1991). Equilibrium beach profiles: characteristics and applications. *Journal of Coastal Research* 7(1), 53–84. https://journals.flvc.org/jcr/article/view/78405
- **Model:** h = A·y^⅔ with A = 0.067·w^0.44 below the step.
- **Used in:** `@simulation/src/coast/fn/Beach.cpp:34`; `docs/handoff/coast.md:133`.
- **Limits:** Dean fitted sand; for gravel it is an extrapolation (`Beach.cpp:34`).

### Ferguson and Church 2004: settling velocity of a grain
Ferguson, R. I., Church, M. (2004). A simple universal equation for grain settling velocity. *Journal of Sedimentary Research* 74(6), 933–937. doi:10.1306/051204740933
- **Model:** fall velocity of natural grains, C1 = 18, C2 = 1, feeding Dean's A.
- **Used in:** `@simulation/src/coast/fn/Beach.cpp:27`.

### Stockdon et al. 2006: wave run-up
Stockdon, H. F., Holman, R. A., Howd, P. A., Sallenger, A. H. (2006). Empirical parameterization of setup, swash, and runup. *Coastal Engineering* 53(7), 573–588. doi:10.1016/j.coastaleng.2005.12.005
- **Model:** the 2 % run-up R = 1.1·(0.35·β·√(H·L₀) + ½·√(H·L₀·(0.563·β² + 0.004))), setting the berm.
- **Used in:** `@simulation/src/coast/fn/Beach.cpp:68`; `docs/simulation.md:101`.
- **Checked by:** `@simulation/test/CoastLake.cpp:55`, `:60`.

### Larson, Erikson and Hanson 2004: banks cut by wave impact
Larson, M., Erikson, L., Hanson, H. (2004). An analytical model to predict dune erosion due to wave impact. *Coastal Engineering* 51(8–9), 675–696. doi:10.1016/j.coastaleng.2004.07.003
- **Model:** a bank of loose ground goes back by 4·Cs·(R − z_toe)²/T per wave, Cs = 1.7 × 10⁻⁴.
- **Used in:** `@simulation/src/coast/fn/Drift.cpp:28`, `:91`; `docs/handoff/coast.md:128`, `:188`.
- **Checked by:** `@simulation/test/CoastLake.cpp:92`.
- **Note:** `docs/handoff/coast.md:188` gives the venue as Marine Geology 211; the paper is in Coastal Engineering 51 ([gaps](../credits.md#gaps)).

## Drift along the shore

### Komar 1998: gross against net drift
Komar, P. D. (1998). *Beach Processes and Sedimentation*, 2nd ed. Prentice Hall, Upper Saddle River, NJ. ISBN 0-13-754938-5.
- **Model:** a symmetric wave climate moves sand both ways along a shore without net drift.
- **Checked by:** `@simulation/test/Coast.cpp:156`; `docs/handoff/coast.md:129`.

### Ashton and Murray 2006: shoreline diffusion and instability
Ashton, A. D., Murray, A. B. (2006). High-angle wave instability and emergent shoreline shapes: 1. Modeling of sand waves, flying spits, and capes. *Journal of Geophysical Research: Earth Surface* 111(F4). doi:10.1029/2005JF000422 (part 2, F04012, unverified)
- **Model:** a shoreline under a low-angle climate diffuses.
- **Checked by:** `@simulation/test/Coast.cpp:157`; `docs/handoff/coast.md:129`.

### Longuet-Higgins 1970: longshore currents (considered, not taken)
Longuet-Higgins, M. S. (1970). Longshore currents generated by obliquely incident sea waves: 1. *Journal of Geophysical Research* 75(33), 6778–6789. doi:10.1029/JC075i033p06778
- **Model:** alongshore thrust of one sea; netting the trains' thrust per cell was rejected because it treats separate storms as one sea.
- **Used in:** `docs/handoff/coast.md:129` (rejected alternative).

## River mouths and sediment transport (held in the coast mirror)

The mouth model is held in `.out/coast/mirror/src/coast/fn/Mouth.cpp` and described in `docs/handoff/coast.md:23`–`27`; line numbers below are in that file.

### Escoffier 1940; O'Brien 1931: an inlet stays open or closes
Escoffier, F. F. (1940). The stability of tidal inlets. *Shore and Beach* 8(4). (unverified: pages, cited as 114–115 and as 28)

O'Brien, M. P. (1931). Estuary tidal prisms related to entrance areas. *Civil Engineering* 1(8), 738–739.
- **Model:** the outflow keeps a mouth open where it moves more sand than the waves lay in it.
- **Used in:** `Mouth.cpp:19` (held); `docs/handoff/coast.md:23`, `:44`, `:135`.

### Moody and Troutman 2002; Leopold and Maddock 1953: the channel a stream keeps
Moody, J. A., Troutman, B. M. (2002). Characterization of the spatial variability of channel morphology. *Earth Surface Processes and Landforms* 27(12), 1251–1266. doi:10.1002/esp.403

Leopold, L. B., Maddock, T., Jr. (1953). *The Hydraulic Geometry of Stream Channels and Some Physiographic Implications*. U.S. Geological Survey Professional Paper 252. doi:10.3133/pp252
- **Model:** width 7.2·Q^0.5 m of the mean flow; depth at a station as Q^0.40 up to bankfull.
- **Used in:** `Mouth.cpp:26`, `:28` (held); `docs/handoff/coast.md:25`.
- **Note:** the 7.2 coefficient was not checked against Moody and Troutman's text.

### Soulsby 1997: grain shear
Soulsby, R. (1997). *Dynamics of Marine Sands: A Manual for Practical Applications*. Thomas Telford, London. ISBN 0-7277-2584-X.
- **Model:** grain shear of a depth-averaged flow over roughness z₀ = D/12.
- **Used in:** `Mouth.cpp:36` (held).

### Meyer-Peter and Müller 1948; Wong and Parker 2006: gravel bedload
Meyer-Peter, E., Müller, R. (1948). Formulas for bed-load transport. *Report of the 2nd Meeting of the International Association for Hydraulic Structures Research*, Stockholm, 39–64.

Wong, M., Parker, G. (2006). Reanalysis and correction of bed-load relation of Meyer-Peter and Müller using their own database. *Journal of Hydraulic Engineering* 132(11), 1159–1168. doi:10.1061/(ASCE)0733-9429(2006)132:11(1159)
- **Model:** q = 3.97·(θ′ − 0.0495)^1.5·√((s − 1)·g·D³), Meyer-Peter and Müller refitted without the form-drag correction.
- **Used in:** `Mouth.cpp:39`, `:40` (held); `docs/handoff/coast.md:26`.

### Rouse 1937: bedload or suspension
Rouse, H. (1937). Modern conceptions of the mechanics of fluid turbulence. *Transactions of the American Society of Civil Engineers* 102(1), 463–505. doi:10.1061/TACEAT.0004872
- **Model:** sand travels as bedload above a Rouse number of 2.5 and wholly in suspension below 0.8.
- **Used in:** `Mouth.cpp:44` (held); `docs/handoff/coast.md:27`.

### Engelund and Hansen 1967; Engelund 1966: sand total load
Engelund, F., Hansen, E. (1967). *A Monograph on Sediment Transport in Alluvial Streams*. Teknisk Forlag, Copenhagen.

Engelund, F. (1966). Hydraulic resistance of alluvial streams. *Journal of the Hydraulics Division, ASCE* 92(HY2), 315–326. doi:10.1061/JYCEAJ.0001417
- **Model:** total load of a sand bed under the dune-bed shear; total shear from grain shear θ′ = 0.06 + 0.4·θ².
- **Used in:** `Mouth.cpp:45`, `:47`, `:59` (held); `docs/handoff/coast.md:27`, `:68`.

The critical Shields stress the mouth uses is credited under [landscape](landscape.md).
