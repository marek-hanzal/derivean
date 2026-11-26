# Atmosphere, climate and calendar — hand-over

Owner of `@simulation/src/{atmosphere,climate,chronology}/**`, `@simulation/test/{Atmosphere,Clouds,Climate,Snow,Plume}.cpp`, `@simulation/tool/{AtmosphereCommand,SnowCommand}.cpp`. Nothing here is committed by this stream; the lead commits.

## 1. Goal

Weather that is caused, never random: a closed water cycle (clouds only from water that evaporated from a real surface, rain only from cloud the wind carried there), seasons, the ten cloud genera derived from the state of the air, a snow line by elevation, and sources (volcanoes, later wind-blown dust) whose ash and gas load the air and dim the sun. Everything is a pure function of seed, calendar time and what was injected — never of thread count or of how time was cut into calls.

## 2. State now

Works, with the test that shows it:

- **Calendar** (`chronology/type/Calendar.hpp`): day, hour, declination, day length, insolation, lunar phase, zero-mean tide. `test/Climate.cpp`.
- **Steady annual climate** (`climate/fn/MoistureBalance.cpp`, called by `solvePlanetClimate` about 140 times inside `evolvePlanet`): precipitation equals evaporation to 0.3 %; seed 42 at 4.6 Gyr gives land mean 0.79 m/yr, wettest cell 2.97 m/yr. `test/Climate.cpp` holds the bounds the geology relies on (land mean 0.3–1.2, no cell above 4, windward coasts wetter, dry planet = no rain, thread independent).
- **Stepped atmosphere** (`atmosphere/**`), 128² cells of 131 km, hourly step (`AirStep`) with two 30-minute wind steps: barotropic vorticity + FFT Poisson; air, upper and surface temperature; vapour, cloud liquid and ice in three terrain-following bands; convection, phase changes, fall-out; soil, runoff, running ocean total. `test/Atmosphere.cpp` runs two years on a hand-built coast planet: water ledger closed to 1e-7 of the water that moves, bit-identical for 2 vs 5 threads and one call vs many, seasons, windward vs lee, valley rain vs summit snow.
- **Clouds** (`fn/Cloud.cpp`, `fn/Explain.cpp`): cover, base, top, optical depth and genus shares as continuous functions of a column. `test/Clouds.cpp` builds one column per genus and a cumulus → cumulonimbus sweep.
- **Snow ladder** (`fn/Snow.cpp`, `constant/Snow.hpp`): 12 rungs per cell, area shares from a mean-preserving triangular hypsometry of the planet heights, `snowAt`, `snowLineOf`. `test/Snow.cpp` plus the snow checks in the two-year run. `terra snow --seed 42 --age 4.6e9` prints the snow line by latitude and at the landing.
- **Plume** (`type/Plume.hpp`, `fn/Plume.cpp`, `fn/AshFall.cpp`): sources of ash (three grain classes), sulphur dioxide, vapour, heat; fine ash, gas and veil carried at four levels; coarse ash as Gaussian patches downwind, booked into the cells they cover; dimming of sunlight; ash on snow that fresh snow buries; the sky is put down and switched off once its planet-mean optical depth is under 1e-7; `ashFallAt` names the source per id. `test/Plume.cpp` (mass closure, one jump vs daily steps on other threads, coarse before fine, footprint along a steady wind, cooling under a veil, nothing without a source). Measured on seed 42, vent at 15° N: a Pinatubo-like injection (1.8e10 kg SO2 at 30 km) gives planet-mean optical depth 0.16 / 0.23 / 0.20 / 0.13 / 0.08 and air temperature −0.22 / −0.30 / −0.42 / −0.35 / −0.24 °C at 30 / 90 / 180 / 365 / 545 days; a small basaltic one (2e8 kg at 8 km) nothing measurable; a caldera-forming one (1e12 kg at 40 km) −1.0 °C after a month, −2.3 °C after a year.

Outside the targets, known:

- **Step cost**: about 78 million instructions per hourly step on one core, 5–6 ms, i.e. 2.5–3 ms per 30 simulated minutes against a 2 ms target; 1.4–2.2 ms wall per hourly step with 15 workers on a loaded machine. A plume with all levels in use adds up to about 60 %.
- **Test time**: the two-year run alone takes about 1 min 50 s under load.
- **Losing the sea**: rain falls to 16–18 % in 6–8 weeks, not below 5 %, because wet ground and snow keep recycling; with those emptied it settles near 5 %.
- **Snow ladder**: the hypsometry only knows 65 km mean heights, so summits above the stretched ceiling have no area in the ledger (landing cell: mean 3338 m, nothing above about 4500 m); every rung gets the same precipitation; sublimation uses the cell's mean ground.
- **Plume**: the veil's dimming is capped at 10.5 % and ash at 60 %, so a caldera-forming eruption cannot cool by more than about 2.5 °C; a source just above the tropopause gets the 400-day veil outright (no blend across it); the patch list has no bound when many sources are active under turning wind; ended sources stay in the list; `addSource` validates nothing; the fixes made after the review were tested but not reviewed again. The eruption figures above were measured before those fixes.
- Rossby waves drift the wrong way on the far side of each pole: a consequence of the triangle-wave latitude.
- Linux is unverified.

## 3. In progress

Nothing is half-done. The last change was the plume and the fixes from its review; all owned tests passed after it from a private build, and the module builds.

## 4. Next steps

1. Blend the injection level over 2–3 km around the tropopause; let dimming grow without a hard cap (for example b·τ / (1 + b·τ)) and re-measure the three eruption sizes.
2. Drop sources that have ended, keeping id and start for the inspector; bound the patch list; validate `addSource` input.
3. Tests still missing: two sources, the seam, clearing of the sky, washout, sooted snow, `advect` under diagonal wind, `solvePoisson`.
4. Re-measure the step cost with a clean sky and with a plume in instructions.
5. Bring the step under 2 ms per 30 simulated minutes: the column physics (`stepWater`, about a third) and the carried fields are where the time goes.

## 5. Decisions and why

- **Hourly physics, 30-minute wind steps**: halves the cost of the column physics; the wind needs the shorter step for stability.
- **Weather systems are a closure**: a single layer cannot be baroclinically unstable, so vorticity of system size grows at a rate set by the simulated temperature gradient × tan(latitude), up to a limit. A two-layer model was rejected: the planetary vorticity changes sign twice along Z.
- **Transport takes as many passes as keep the Courant sum under 0.5**: the unsplit second-order fluxes are unstable above about 0.55 under diagonal wind (found in review).
- **Orography acts through capacity**: air over ground holds what air lifted by `terrainLift` (at most 400 m) can hold. Full ground height made windward cells take tens of metres of rain a year.
- **Steady solver**: rain per lifting episode is limited to the excess the episode finds, and a ceiling of 4 m/yr is a last guard; without them single cells received over 100 m/yr.
- **Snow rungs use the constant `LapseRate`**, not the column's lapse (mean 5.5 °C/km): with the column lapse the pack's snow line sat 300–500 m from where `weatherAt` shows snow falling.
- **`SnowShade` 0.45 → 0.2**, and only cover beyond `snowUsual` counts: at 0.45 the landing cell locked itself under snow and was colder in July (2.6 °C at sea level) than in January.
- **`setSurface` never makes snow**: packs on ground that newly counts are thinned; decreases go to the ocean.
- **Coarse ash is analytic, fine ash is on the grid**: lapilli and sand-size ash fall within hours and within some hundred kilometres; one landing place per step at height × wind ÷ settling speed, spread 0.3 of the distance. The hand-over is by grain class, not by a fixed distance.
- **Sulphur is counted as the dioxide throughout**, so gas + veil + fallen = emitted.
- Traps: stores that only shrink by a share never reach zero (flush below a trace); unscoped enumerators share `derivean::sim` with every other stream; other streams' in-progress files break the module build several times a day — a private build of the owned sources against the last good library avoids waiting.

## 6. Contracts with other streams

All in `atmosphere/type/Atmosphere.hpp` unless noted.

- `startAtmosphere(planet, calendar, workers)`, `advanceAtmosphere(atmosphere, calendar, workers)` (absolute date, whole steps), `setSurface(atmosphere, elevation, seaLevel)` (planet-resolution heights).
- `waterLedger(atmosphere)` → kg per store; `total` is constant except that it grows by `plumeLedger(...).vapourEmitted`. `oceanVolume(atmosphere)` → m³ for `Planet::surfaceWater`.
- `weatherAt(atmosphere, position, groundElevation, calendar)` → temperature, rain, snow (mm/h), cloud cover, cloud water, humidity, wind, `snowpack` (kg/m² at that ground).
- `inspectWeather(...)` → column, profile, clouds per band, winds per band, humidity, pressure, dew point, snow line, ledger, `ash`, `aerosolDepth`, and `cause` (one sentence; an ash clause is appended where ash fell).
- `cloudField(atmosphere)` (`type/Cloud.hpp`): per band cover, base, top, optical depth, ice share, wind, genus; `heaping`, `rain`, `snow`; `snowLadder` (float kg/m², `[cell · SnowRungs + rung]`, `cell = z · size + x`), `snowLine` (m above `seaLevel`), `seaLevel`; `aerosolDepth`, `ashAloft` (kg/m²), `ashLevel` (m above `seaLevel`). `SnowLadder` elevations are in `constant/Snow.hpp`.
- `snowAt(atmosphere, position, elevation)` → kg/m²; `snowLineOf(atmosphere, x, z)`.
- `sunDirectionAt(position, calendar)` → unit vector in world axes.
- Sources (`type/Plume.hpp`): `struct AirSource { id, position, height (m above the sea), start, end (calendar s), ash[3] (kg/s: lapilli, grit, dust), sulphur (kg/s SO2), vapour (kg/s), heat (W) }`; `addSource(atmosphere, source)` — only what lies after the atmosphere's current step is emitted; `ashFallAt(atmosphere, position)` → `{mass kg/m², rate kg/m²/s, source, since}`; `plumeLedger(atmosphere)`.
- Climate helpers for geology: `solvePlanetClimate`, `solveMoisture`, `zonalClimate`, `beltWind`, `storminess`, `northwardZ`, `annualSnowBalance`, `frostCracking`, `positiveDegreeDays`, `seasonalTemperature`, `stepSnowpack`.

## 7. Build, run, verify

From the worktree root:

- `mise -C @simulation run build` — library, `terra`, tests.
- `mise -C @simulation run test` — all streams' tests; mine are `testClimate`, `testClouds`, `testAtmosphere` (two years, about 1 min 50 s under load), `testSnow` (about 3 s), `testPlume` (about 20 s).
- `.out/simulation/build/macosx/arm64/release/terra atmosphere --seed 42 --age 4.6e9 --day 200 --out DIR` — maps (cloud per band, genus, precipitation, vapour, wind, temperature, snow), ledger, genus shares, step timing.
- `terra snow --seed 42 --age 4.6e9` — snow line table and landing cell.
- `mise run check` at the root also needs the engine and game to compile.
- Step cost is measured in instructions with `/usr/bin/time -l` on a small program that takes N extra steps; wall times on this machine are not repeatable.

## 8. Open questions

- Should a session's water ledger hand perennial snow over to `Tile::ice`? Today a pack above 3000 kg/m² sheds to runoff.
- What fine-ash share should the volcanism stream pass as `ash[Dust]`? Ten per cent of the erupted mass gives a planet-wide ash cloud for two weeks; real fine ash mostly aggregates and falls with the coarse.
- Does the engine advance the atmosphere before or after it adds sources? A source added after its start loses the part already past.
- Is two years of weather in the unit suite acceptable at its current cost?
