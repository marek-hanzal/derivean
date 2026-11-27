# Credits: ice sheets, glacial erosion and firn

Part of [Credits](../credits.md); the entry format is explained there.

## Flow and erosion

### Glen 1955: the flow law of ice
Glen, J. W. (1955). The creep of polycrystalline ice. *Proceedings of the Royal Society of London, Series A* 228(1175), 519–538. doi:10.1098/rspa.1955.0066
- **Model:** creep with stress exponent n = 3; the depth-averaged flux of a slab as CreepFlow · H⁴ · slope³.
- **Used in:** `@simulation/src/icesheet/constant/Ice.hpp:13`; `docs/handoff/icesheet.md:176`, `:294`.
- **Note:** the rate factor (5 × 10⁻¹⁷ Pa⁻³/yr) and the sliding coefficient carry no source ([gaps](../credits.md#gaps)).

### Herman et al. 2015: erosion by sliding ice
Herman, F., Beyssac, O., Brughelli, M., et al. (2015). Erosion by an Alpine glacier. *Science* 350(6257), 193–195. doi:10.1126/science.aab2386
- **Model:** bed lowering as 2.7 × 10⁻⁷ · u² m/yr with u the sliding speed.
- **Used in:** `@simulation/src/landscape/fn/Glacial.cpp:18`; `docs/handoff/relief.md:136`.

### Boulton and Hindmarsh 1987: viscous deforming till
Boulton, G. S., Hindmarsh, R. C. A. (1987). Sediment deformation beneath glaciers: rheology and geological consequences. *Journal of Geophysical Research* 92(B9), 9059–9082. doi:10.1029/JB092iB09p09059
- **Model:** deforming depth as N^−1.8, 0.5 m at 50 kPa (Breiðamerkurjökull). The valley glaciers of the finer levels carry their loose bed as a layer of that depth moving at half the sliding speed, 0.5 · u · h m²/yr per metre of width.
- **Used in:** `@simulation/src/landscape/fn/Glacial.cpp:23`, `:177`; `docs/handoff/relief.md:138`; `docs/handoff/icesheet.md:285`; rig `.out/icesheet/drumlin/` (not delivered).
- **Approximations:** the depth is held at 0.5 m whatever the effective pressure; at 4–64 m cells and 20–100 kyr steps the capacity exceeds any cover, so a sliding glacier sweeps its bed of loose cover in a cold step.

## The bed under the ice (design and rigs, not in the tree)

### Tulaczyk, Kamb and Engelhardt 2000; Iverson, Hooyer and Baker 1998: Coulomb-plastic till
Tulaczyk, S., Kamb, W. B., Engelhardt, H. F. (2000). Basal mechanics of Ice Stream B, West Antarctica: 1. Till mechanics. *Journal of Geophysical Research* 105(B1), 463–481. doi:10.1029/1999JB900329

Iverson, N. R., Hooyer, T. S., Baker, R. W. (1998). Ring-shear studies of till deformation: Coulomb-plastic behavior and distributed strain in glacier beds. *Journal of Glaciology* 44(148), 634–642. doi:10.3189/S0022143000002136
- **Model:** a till bed yields at μ · N, the effective pressure; weighed as the alternative to the viscous bed.
- **Used in:** `docs/handoff/icesheet.md:90` (analysis only).

### Hindmarsh 1998; Fowler 2000; Fowler 2009; Fowler and Chapwanya 2014; Stokes et al. 2013: drumlin instability
Hindmarsh, R. C. A. (1998). Drumlinization and drumlin-forming instabilities: viscous till mechanisms. *Journal of Glaciology* 44(147), 293–314. doi:10.3189/S002214300000263X

Fowler, A. C. (2000). An instability mechanism for drumlin formation. In: *Deformation of Glacial Materials*, Geological Society, London, Special Publications 176, 307–319. doi:10.1144/GSL.SP.2000.176.01.23

Fowler, A. C. (2009). Instability modelling of drumlin formation incorporating lee-side cavity growth. *Proceedings of the Royal Society A* 465(2109), 2681–2702. doi:10.1098/rspa.2008.0490

Fowler, A. C., Chapwanya, M. (2014). An instability theory for the formation of ribbed moraine, drumlins and mega-scale glacial lineations. *Proceedings of the Royal Society A* 470(2171), 20140185. doi:10.1098/rspa.2014.0185

Stokes, C. R., Fowler, A. C., Clark, C. D., Hindmarsh, R. C. A., Spagnolo, M. (2013). The instability theory of drumlin formation and its explanation of their varied composition and internal structure. *Quaternary Science Reviews* 62, 77–96. doi:10.1016/j.quascirev.2012.11.011
- **Model:** drumlins as an instability of the ice–till–water system, growing at a preferred spacing of a few hundred metres and stretching with ice speed; the linear stage of Fowler and Chapwanya was verified in a rig, the nonlinear stage needs Fowler 2009's cavities and was parked (lead, 26 Nov).
- **Used in:** `docs/handoff/icesheet.md:286`, `:289`, `:298`, `:300`; `docs/handoff/README.md:60`; rig `.out/icesheet/drum/`.
- **Note:** `icesheet.md:286` cites a UPSpace repository handle for Fowler and Chapwanya; the DOI above is the journal's.

## Glacial cycles

### Walker et al. 2009: the base of the Holocene
Walker, M., Johnsen, S., Rasmussen, S. O., Popp, T., Steffensen, J.-P., Gibbard, P., Hoek, W., Lowe, J., Andrews, J., Björck, S., Cwynar, L. C., Hughen, K., Kershaw, P., Kromer, B., Litt, T., Lowe, D. J., Nakagawa, T., Newnham, R., Schwander, J. (2009). Formal definition and dating of the GSSP (Global Stratotype Section and Point) for the base of the Holocene using the Greenland NGRIP ice core, and selected auxiliary records. *Journal of Quaternary Science* 24(1), 3–17. doi:10.1002/jqs.1227
- **Model:** Earth's present interglacial began 11 700 yr b2k. A finer level ends its window with the warm steps the world's own interglacial spans since `lastTermination`, at least one; Earth's span is shorter than one step of the Local (100 kyr) or Site (20 kyr) level, so the tail is one step there, not the last fifth of the window.
- **Used in:** `@simulation/src/landscape/fn/Replay.cpp:34`; `docs/handoff/relief.md:140`.

## Firn (approved, not yet written)

### Herron and Langway 1980: firn densification
Herron, M. M., Langway, C. C. (1980). Firn densification: an empirical model. *Journal of Glaciology* 25(93), 373–385. doi:10.3189/S0022143000015239
- **Model:** densification of seasonal snow to firn (from about 550 kg/m³) and to glacier ice, from temperature and accumulation.
- **Used in:** planned, `docs/handoff/engine.md:154`.
