# Credits: landscape and erosion

Part of [Credits](../credits.md); the entry format is explained there. Erosion by ice is under [ice](ice.md), dunes under [aeolian](aeolian.md), the water balance that drives the rivers under [hydrology](hydrology.md).

### Shields 1936: the critical shear stress of a grain
Shields, A. (1936). *Anwendung der Ähnlichkeitsmechanik und der Turbulenzforschung auf die Geschiebebewegung*. Mitteilungen der Preußischen Versuchsanstalt für Wasserbau und Schiffbau, Heft 26, Berlin. English translation by W. P. Ott and J. C. van Uchelen, U.S. Soil Conservation Service (1936).
- **Model:** a grain moves once the dimensionless bed shear exceeds a critical value (0.045 used); the largest stone a stream moves on a bed of a given fall.
- **Used in:** `@simulation/src/ground/constant/Ground.hpp:24`; held in `.out/coast/mirror/src/coast/fn/Mouth.cpp:42`; `docs/simulation.md:44`.
- **Note:** the value 0.045 is a common reading of Shields' curve for gravel; which later fit it comes from is not stated ([gaps](../credits.md#gaps)).

### Hack 1957: stream length against drainage area
Hack, J. T. (1957). *Studies of Longitudinal Stream Profiles in Virginia and Maryland*. U.S. Geological Survey Professional Paper 294-B. doi:10.3133/pp294B
- **Model:** L = 1.4·A^0.6 (miles, square miles); the length of channel above a point that carries a given flow.
- **Used in:** `@simulation/src/ground/constant/Ground.hpp:27`; the flow-duration lag in [hydrology](hydrology.md).

### Budyko 1974: runoff from rain and warmth
See [hydrology](hydrology.md#budyko-1974-the-mean-water-balance); used by the landscape's `runoffOf` in `@simulation/src/landscape/fn/Forcing.hpp:51`.

### Barnes, Lehman and Mulla 2014: priority flood
Barnes, R., Lehman, C., Mulla, D. (2014). Priority-flood: an optimal depression-filling and watershed-labeling algorithm for digital elevation models. *Computers & Geosciences* 62, 117–127. doi:10.1016/j.cageo.2013.04.024
- **Model:** cells taken in rising order of water level from the open rim, so every depression fills to its spill point and drains to its outlet; the bucket queue of the integer case.
- **Used in:** `@simulation/src/landscape/fn/Flood.cpp:11`, `@simulation/src/landscape/fn/Drainage.cpp:58`, `@simulation/src/landscape/fn/TrunkFlow.hpp:19`; `docs/simulation.md:71`.
- **Note:** the code names the method but cites no paper; this is the standard reference for it, added here, not a source the code's author named.

### Stream-power incision
`@simulation/src/landscape/fn/Process.hpp:50`, `@simulation/src/landscape/fn/Fluvial.cpp:50` and `@simulation/src/landscape/fn/Trunks.cpp:18` use stream-power incision with transport-length deposition and coefficients by rock (sandstone 3 × 10⁻⁶, gneiss 1 × 10⁻⁶ m^(1−2m)/yr). No paper is named for the law, the transport-length term or the coefficients ([gaps](../credits.md#gaps)).

The landscapes the relief is compared with are under [reference data](reference-data.md).
