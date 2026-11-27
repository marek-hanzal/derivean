# Credits

DeRivean stands on the work of people who spent careers watching rivers carry sand, measuring how lava stops, how waves climb a beach, how haze takes up water, how the Moon drew away from the Earth. Their papers, datasets, books and standards give the simulation its laws and numbers, and their measurements are what it is checked against. This is our thanks to them, and the record of what we took.

The rule is in [AGENTS.md](../AGENTS.md): every source the implementation builds on gets an entry here in the same change that first uses it, and the code comment or document that applies it cites the entry. A citation and a licence are different things: what lets us ship someone's code is recorded under [libraries](credits/libraries.md).

This catalogue was first assembled on 26 Nov 2026 from the code comments, the documentation, the hand-overs, the review mailbox and the commit history. Bibliographic data were checked against publisher, DOI and catalogue records; what could not be confirmed is marked (unverified). Line numbers are as of commit `1c38a06`, after the geometry (`cb87997`) and astronomy (`e3ed117`) slices landed; they drift, the file paths do not.

## How to read an entry

Each entry names the source, then:

- **Model:** the law, method or numbers the code takes from it.
- **Used in:** where it is applied, as `path:line`. Work that is not in the tree yet says so: *held* (written in a stream's patch or mirror under `.out/`, waiting to land) or *planned* (designed in a hand-over, not written).
- **Checked by:** the test or tool that compares the implementation with the source or with measurement.
- **Reference data:** for measurements the planet is compared with rather than models it implements.
- **Approximations / Limits / Note:** what we knowingly simplified, where the model stops holding, or a doubt about the citation.

The aim is that every physical model carries its source, the model taken, where it is implemented, its assumptions and approximations, the data and tests that validate it, the error it is allowed and what it cannot represent. Entries carry those fields where the code or the hand-over states them; assumptions, tolerances and limits are not yet recorded for most models.

## Domains

| Domain | File | Entries |
| --- | --- | --- |
| Planet, tectonics and earthquakes | [credits/planet.md](credits/planet.md) | 6 |
| Landscape and erosion | [credits/landscape.md](credits/landscape.md) | 5 |
| Hydrology and groundwater | [credits/hydrology.md](credits/hydrology.md) | 18 |
| Coasts, waves and river mouths | [credits/coasts.md](credits/coasts.md) | 19 |
| Wind-blown sand and dust | [credits/aeolian.md](credits/aeolian.md) | 4 |
| Volcanism | [credits/volcanism.md](credits/volcanism.md) | 12 |
| Ice sheets, glacial erosion and firn | [credits/ice.md](credits/ice.md) | 7 |
| Atmosphere, haze, clouds and snow | [credits/atmosphere.md](credits/atmosphere.md) | 33 |
| Astronomy and the geometry of the sphere | [credits/astronomy-geometry.md](credits/astronomy-geometry.md) | 28 |
| Rendering | [credits/rendering.md](credits/rendering.md) | 8 |
| Engine, libraries and tools | [credits/libraries.md](credits/libraries.md) | 21 |
| Reference data for comparison with Earth | [credits/reference-data.md](credits/reference-data.md) | 12 |

An entry may group the papers of one model; together the files cite about 190 works, 15 software projects and 6 platform documents.

## Not yet identified

Sources the code or documents name, whose exact identity could not be settled:

- **Larson et al. 2009**, `docs/handoff/coast.md:188`: a re-use of the wave-impact coefficient Cs = 1.7 × 10⁻⁴. The likeliest match is Larson, M., Donnelly, C., Jiménez, J. A., Hanson, H. (2009). Analytical model of beach erosion and overwash during storms. *Proceedings of the ICE – Maritime Engineering* 162(3), 115–125, doi:10.1680/maen.2009.162.3.115; whether it uses that Cs was not checked.
- **Hydrological Atlas of Switzerland** (Lake Geneva, 0.94–0.95 m/yr) and **USGS** (Great Salt Lake, 1.0–1.15 m/yr), held `.out/groundwater/evap/patched/@simulation/test/Evaporation.cpp:20`, `:21`: no plate or report named.
- **The five climate stations** (Aswan, Las Vegas, Salt Lake City, Geneva, Entebbe) that the evaporation's sunshine and humidity are fitted to, held `Evaporation.cpp:14`; `docs/handoff/groundwater.md:124`: the normals' source is not named.
- **Pinatubo 1991 and St Helens 1980** volumes and caldera width, `@simulation/src/volcanic/constant/Rheology.hpp:67`: no source.
- **North 1975**, held `Evaporation.cpp:11`: which of North's two 1975 papers (see [hydrology](credits/hydrology.md)).
- **Botter et al. 2007**, held `.out/coast/mirror/src/hydrology/fn/FlowDuration.hpp:16`: which of the group's two 2007 papers (see [hydrology](credits/hydrology.md)).
- **USGS on drainage density**, `docs/reference-landscapes.md:270`, `:806`: a publication page, its paper's authors and year not recorded.
- **Simon et al. 1994** (Simon, J. L., Bretagnon, P., Chapront, J., et al., Numerical expressions for precession formulae and mean elements for the Moon and the planets, *Astronomy and Astrophysics* 282, 663–683): reported by the lead as an S2 source, but neither the astronomy code nor `docs/handoff/astronomy.md` cites it; not entered until a place that uses it is named.

## Gaps

Places where the code uses a law or a number whose citation is missing or imprecise. The code was not changed while compiling this list; each item is for the owning stream.

Named without a work or year (entries added here; the comment should cite them):

- `@simulation/src/volcanic/fn/Unrest.cpp:38` "Mogi"; `@simulation/src/volcanic/fn/Chamber.cpp:58` "Box and Muller".
- `@simulation/src/aeolian/fn/Regime.hpp:9` "Bagnold"; `@simulation/src/aeolian/fn/Regime.hpp:13`, `Regime.cpp:15` "Lettau and Lettau".
- `@simulation/src/atmosphere/fn/Circulation.cpp:15` "Robert–Asselin–Williams", `:62` "Arakawa's Jacobian".
- `@simulation/src/climate/fn/Moisture.hpp:10` "the Magnus formula" (the coefficients at `:35` are Alduchov and Eskridge 1996).
- `@simulation/src/atmosphere/fn/Haze.cpp:44` "κ-Köhler", `:47` "Stokes"; `@simulation/src/atmosphere/fn/Haze.cpp:11` Mie theory, with no record of the code or tables the constants were computed with.
- `@simulation/src/icesheet/constant/Ice.hpp:13` "Glen's law"; `@simulation/src/ground/constant/Ground.hpp:24` "Shields", `:27` "Hack's law"; `@simulation/src/hydrology/constant/Water.hpp:9` "Manning"; `@simulation/src/landscape/fn/Forcing.hpp:51` "Budyko".
- `@game/src/sky/fn/skyLight.ts:55` "Koschmieder"; `@simulation/src/planet/constant/Scale.hpp:18` "Airy isostasy".
- `@game/src/clouds/fn/marchNode.ts:88` "Henyey-Greenstein"; `@game/src/water/fn/sheetSurface.ts:36` "Schlick"; `@game/src/clouds/constant/noise.ts:60` "Perlin", "Worley" (which variant not stated).
- `@simulation/src/landscape/fn/Flood.cpp:11`, `Drainage.cpp:58`, `TrunkFlow.hpp:19` priority flood: no paper; Barnes et al. 2014 is credited as the standard reference.
- `@simulation/src/hydrology/fn/Surge.cpp:20`: local-inertia shallow water with the discharge blended along the line of faces, no paper named; likely Bates, Horritt and Fewtrell 2010 (*Journal of Hydrology* 387, 33–45, doi:10.1016/j.jhydrol.2010.03.027) and de Almeida et al. 2012 (*Water Resources Research* 48; unverified), not entered until the stream confirms.
- `@simulation/src/groundwater/constant/Hydraulics.hpp:43`: a joint set conducting by the cube of its aperture (the cubic law; likely Snow 1965 and Witherspoon et al. 1980, unverified), with no paper named and the 1 × 10⁻⁶ m/s reference unsourced.

Numbers or laws with no source at all:

- `@simulation/src/landscape/fn/Process.hpp:50`, `Fluvial.cpp:50`, `Trunks.cpp:18`: stream-power incision, transport-length deposition and the erodibility by rock (sandstone 3 × 10⁻⁶, gneiss 1 × 10⁻⁶).
- `@simulation/src/icesheet/constant/Ice.hpp`: Glen's rate factor 5 × 10⁻¹⁷ Pa⁻³/yr and the sliding coefficient 5 × 10⁻⁹ m/yr/Pa² (`docs/handoff/icesheet.md:176`).
- `@simulation/src/hydrology/constant/Water.hpp:9`: Manning's n = 0.035 of a natural bed; `@simulation/src/ground/constant/Ground.hpp:24`: critical Shields stress 0.045; `Ground.hpp:27`: Hack's coefficient and exponent.
- `@simulation/src/landscape/fn/Forcing.hpp:56`: the evaporative demand 0.25 + 0.035·T m/yr of the closed-form `runoffOf`.
- `@simulation/src/climate/fn/Zonal.cpp:59`: the cooling surface 288 + 1612·exp(−t / 24.4 Myr) K of the young planet.
- `@simulation/src/planet/fn/Contacts.cpp:8`, `:16`: arc yield and hotspot gain, tuned to Earth's hypsometry, not taken from a source.
- `@game/src/sky/fn/clearSky.ts:9`: the sky light falling tenfold per 2.6° of a sinking sun in civil twilight.
- `@simulation/src/climate/fn/Zonal.cpp:86`: the zonal mean temperature 27 − 49 sin²φ °C, apparently fitted to Earth's.
- `@simulation/src/atmosphere/fn/Heat.cpp:26` (lapse 6.5 °C/km) and `@simulation/src/atmosphere/constant/Light.hpp:17` (scale height 8 km): standard-atmosphere values (U.S. Standard Atmosphere 1976 or ISO 2533) without a citation.
- `@simulation/src/material/definition/*.hpp` (43 materials): densities, strengths, cohesion, friction, porosity, conductivities and melting points, none with a source.

Citations to correct:

- `docs/handoff/coast.md:188`: Larson, Erikson and Hanson (2004) is in *Coastal Engineering* 51, not *Marine Geology* 211.
- `docs/reference-landscapes.md:801`: the "Körner et al. 2021" link (boris.unibe.ch/158972) is Körner, Urbach and Paulsen 2021, "Mountain definitions and their consequences"; the inventory of mountains is Körner et al. 2017. The PDF path may also be wrong (the file `2021_AlpineBot.pdf` appears under handle 158030).
- `docs/handoff/volcanic.md:24`: Earth's eruption rates first quoted "from memory"; line 25 then reads them from White, Crisp and Spera 2006.
- Held evaporation: `docs/handoff/groundwater.md:124` says a lake at 3.8 km evaporates about 22 % short with γ at sea level, `.out/groundwater/evap/patched/@simulation/src/hydrology/fn/Evaporation.cpp:24` says a lake at 3 km about a third short.
