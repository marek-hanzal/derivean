# Hand-over

This directory lets a person or model with none of the original context continue the work on the branch `claude/derivean-landscape-engine-e64dc8`. It describes the present state; history is in Git. The lead keeps this file; every workstream keeps its own file next to it and updates it at each milestone.

Read in this order: [AGENTS.md](../../AGENTS.md), the owner's briefs in [brief/](brief/) ([world simulation](brief/world-simulation.md), [clouds](brief/clouds.md)), this file, then the file of the stream you continue. The model itself is described in [simulation.md](../simulation.md); what real ground looks like, with numbers, in [reference-landscapes.md](../reference-landscapes.md).

## What the owner decided during the work

These are binding and are not in the original briefs.

- **Look comes first.** The world must not read as a height map: no rounded forms everywhere, no uniform country. Edges, walls, canyons, rough inaccessible places, massive ranges with broad valleys under them, and different kinds of country as one travels. "A rock massif must be a massif, not a heap of hillocks."
- **Nothing is placed afterwards.** Stones, crags, cascades, hot-spring ground and every other feature must arise inside the simulation and be explainable. No props, no noise that shapes relief, no switched "modes" that stamp a landform where a classifier says so.
- **Measured against Earth.** Compare captures with photographs of real (plant-free) ground and the planet with the figures in the reference catalogue. The world has no life.
- **Cubes at every distance.** Far terrain is the voxel world at coarser cube sizes, drawn by the same pipeline as near voxels. No smooth terrain meshes, no interpolated normals.
- **WebGPU.** The renderer moves from WebGL to three's `WebGPURenderer`; the WebGL path is deleted at the flip.
- **View distance and fill first, optimisation last.** At least 250 km of view; generous budgets now, pulled down only when rendering cannot hold.
- **All cores.** Native threads for everything that can run in parallel. No GPU compute for the authoritative simulation (results must be bit-identical on every machine); GPU compute is for presentation.
- **Weather is causal.** Clouds come from water that evaporated; snow lies above a simulated snow line; nothing in the sky is random.
- **Volcanoes live during play.** A vent keeps a deterministic eruption schedule in calendar time; an eruption changes terrain, loads the atmosphere and leaves an aftermath. Frequency may be raised only through physical inputs (melt supply, magma properties), never through a multiplier.
- **Missing processes get built**: coasts and lake shores, wind and dry lands, volcanism, ice sheets, karst.
- **In-game map** on `M` (opens and closes).
- Window title stays "DeRivean · Fable". The fixture demo ("The Stacked Channels") is gone; only the simulated world matters.

## State of the tree

Last checkpoint commit: `38b4939` (work in progress, not a verified state). Later work is uncommitted unless a newer commit exists.

On 26 Nov at about 03:00 every running stream stopped mid-work on a model usage limit. Each was restarted by a fresh worker from its own file here and the checkpoint `c131234`; a stream's file can lag behind its code, so a successor first compares `git diff 54fdbd9 c131234` over the stream's directories and finishes or undoes half-done edits. The orphaned run `terra volcanic --x 13686236 --z 11027538 --level 2` (six hours of CPU without finishing) was stopped; the volcanic stream found and fixed its cause (oversized minor eruptions, millimetre relaxation of cones on steep ground).

`mise -C @simulation run build` reconfigures the shared build directory every time; a stream that builds privately sets its own `XMAKE_CONFIGDIR` (the `xmake` on the path is a mise shim that otherwise writes the shared configuration).

Starting the game builds the planet first: about 8 minutes at 08:54 and still unfinished after 25 minutes at 09:30 (load 35–52, other games running). Whether the tree got slower or only the machine is not established; a cached planet or a faster build is an engine item.

Thin vertical columns, one cell wide and up to hundreds of metres tall: first traced to a stale replay of 02:52–03:03 (one-cell needles of 2.2 km of loose cover from the old `settleDebris`, fixed in the relief stream), but the owner saw them again at 10:45 in a live game at the spawn (x −2521902, z −4671906) on a build after that fix. Open; under investigation with a scan of the whole view.

The strata column is now the one at the start of the region window (`strata/fn/Window.cpp`); before, the Region read the planet's present cover and counted the window's erosion twice, so ranges showed basement only. See planet.md.

- `mise -C @simulation run build` builds when no stream is mid-save; streams share one working tree, so a failing build is usually someone's half-saved file — wait and retry before touching another owner's code.
- `mise -C @simulation run test` (whole suite, 25 min, binary of 10:57 with the relief stream's uncommitted river-floor and ice edits): `testChart` fixed (it now centres the local chart on the edge of the finished Local tiles); it stops at the intervention lake cascade ("no open stream bed with dry ground for the works in the middle of the site", `intervention/fn/ScenarioLakes.cpp`). The relief stream is bisecting which change caused it; everything after it is unverified.
- Root `mise run check` fails in `engine-tests` (`Planet.cpp:36`, the player must come to rest on the ground: the engine places the player on the Site surface while voxels come from the finer Ground level).
- The renderer is flipped: one WebGPU scene draws voxel chunks and far cube levels 2–13, with sun shadows, snow, clouds, cloud shadow and precipitation; the WebGL path is gone (three's WebGL 2 fallback backend draws no clouds). Captures under `.out/playwright/renderer/`. Captures are still not taken on final data until the engine's readiness is truthful.

## Workstreams

| Stream | File | Owns | Status |
| --- | --- | --- | --- |
| Planet, strata, rock mass, ore, heat, spawn | [planet.md](planet.md) | `@simulation/src/{planet,strata,rockmass,ore,tectonics,core}`, `simulation/fn/Spawn.*`, `simulation/{type,service}/World.*`, `tool/PlanetCommand.cpp` | Lead's own; stable |
| Relief of Region, Local, Site | [relief.md](relief.md) | behaviour of those levels in `@simulation/src/landscape/**`, `tool/{View,Relief,Block}Command.cpp` | In progress, top priority. Comb and debris needles gone (Local and Site held to the parent's mean, Region drainage rule at Local); the range now reads as smooth rounded upland (mean slope 21.6°, faces above 60° 0.45 %), so walls are next: incision to threshold slopes by rock-mass strength, ice that sharpens |
| Ground level and voxel forms | [ground.md](ground.md) | `@simulation/src/{ground,relief}`, `simulation/fn/{Query,Reader,Live}.*` | Finished; no active owner |
| Groundwater, springs, caves, karst | [groundwater.md](groundwater.md) | `@simulation/src/groundwater/**` | In progress; stand-in deleted, real springs and caves linked; world runs and second review next |
| Interventions, hydrology, time | [interventions.md](interventions.md) | `@simulation/src/{hydrology,intervention}` | Finished; not wired into the game |
| Atmosphere, climate, calendar | [atmosphere.md](atmosphere.md) | `@simulation/src/{atmosphere,climate,chronology}` | Finished: snow by elevation, sources for ash, gas and dust with transport and fallout; nothing emits yet |
| Coasts and lake shores | [coast.md](coast.md) | `@simulation/src/coast/**` | Waves, cliffs with benches and sea caves, drift with spits, barriers and lagoons; seed 42 sea coast with islands and bays; waves grow along the depth-limited SPM curve (shallow 3.2 m water at 20 m/s: 0.84 m, Balaton-like); open: sand laid 24–37 % thicker near tile edges (drift boundary), shore length 543 → 391 km and Region platforms p50 95 → 398 m under the new law, not yet judged; no lake with a beach found, because lakes shallower than 45 m are filled when the finer level is built (level consistency); `terra coast --find 1` |
| Wind and dry lands | [aeolian.md](aeolian.md) | `@simulation/src/aeolian/**` | Shelter only behind breaks steeper than 20°, plants hold sand where rain reaches 0.3 of evaporation demand, far dust traced back along the planet's mean wind (2.4 m loess on the trade-wind tile), dune pass on the pool (bit-identical across thread counts). No erg anywhere on seed 42: the Region's hidden-stream wash takes 99–100 % of each step's cover on arid plains (relief stream to make it follow effective desert runoff), and freed sand drowns or leaves the tile; the transmission-loss hook `sinkIntoBed` waits for wiring in `routeWater`. Next: far-dust seams and calibration, a closed arid basin for an erg (or a planet finding that none exists). Wind costs +1.4 / +2.3 / +2.2 s per Region / Local / Site tile |
| Volcanism | [volcanic.md](volcanic.md) | `@simulation/src/volcanic/**`, the deposit edit in `@simulation/src/intervention/**`, voxel piles in `relief/fn/{Form,Ground}.cpp` and `simulation/fn/Query.cpp` | World tiles build (an arc Local tile with its parents in about 4 min); piles in the voxel answer; live eruption functions exist (`outbreaksBetween`, `airSourcesOf`, `layOutbreak`), nothing calls them yet; eruptions 4.1 a year on land, 5–6× below Earth, under analysis |
| Ice sheets | [icesheet.md](icesheet.md) | `@simulation/src/icesheet/**` | Region sheets on real land; last sheet at Local and Site (basins, till tails, eskers, moraine) with tests; seam step at Local, speckle lakes and grid dashes open; Site not built yet |
| Level consistency: lakes, rivers, seams | [consistency.md](consistency.md) | `@simulation/src/landscape/**` files the relief stream is not changing, `test/LevelConsistency.cpp` | Started: lakes the parent has must stay lakes at finer levels (the owner's lake with a sandy beach), rivers keep water and course across tile edges and levels, tile seams at the root |
| Measurements, inspector, scouting | [measure.md](measure.md) | `@simulation/src/{measure,inspect}`, `tool/Measure*`, `tool/InspectCommand.cpp` | Finished a full pass; to be repeated |
| Engine | [engine.md](engine.md) | `@engine/**`, `@napi/**`, `@game/main/engine*`, `@game/contract/GameBridge.ts`, `@game/preload/**` | In progress; long queue |
| Renderer | [renderer.md](renderer.md) | `@game/src/**` except the map, `@game/main/{index,capture}.mjs`, `@game/capture/**` | WebGPU only: cubes at every distance, sun shadows, snow, clouds and cloud shadow ported, rain and snow written; next: clouds in motion and live, the clouds' GPU cost, low-sun captures of a range |
| Map | [map.md](map.md) | `@game/src/map/**`, `@simulation/src/chart/**` | Finished; no active owner. Review fixes in (stale reads, world change, lost context); an open map costs no more frame time than the closed game; `M` last confirmed by real input before those fixes — confirm on a quiet machine with the steps in map.md |

A stream whose file is missing has not written it yet; its state is then only in the code.

## Defects that decide the result, most important first

1. **Relief mechanics.** Region in high ranges was a cell-scale maze (fixed in the rework: channel initiation, limiting gradient solved in the step); the rework then locked gullies to grid rows on a featureless plane (open regression); Local and Site still show the old faults (mean slope 57° at Local in ranges, a square kilometre at 75° at Site, soil domes in gentle country). Rivers lose water at Region tile edges, a river is not the same river at the next level, lakes cover 15 % of the land, kilometres of loose cover sit under some Site tiles, the Site ledger does not close.
2. **The game hides the world.** Fill does not keep up with the view; far terrain is smooth; the far-cube contract is not in `GameBridge.ts` yet; the engine's voxel crew used a third of the cores.
3. **The middle of the roughness distribution is missing**: land is either flat (42 % plains against Earth's 25 %) or extreme; old worn mountains, canyon lands and open basins have no site on seed 42.
4. **Processes that were absent** (now started): coasts, wind, volcanism, ice sheets, karst. Until they land, the catalogue types that need them have no site.
5. **Determinism**: an authoritative survey near a tile edge depended on whether the neighbour existed (reported fixed by the ground stream; re-verify).
6. **Planet**: more low land than Earth after carbonate was introduced (38 % below 200 m against 29 %); the crust stock grows without bound beyond 4.6 Gyr; 37 % of the coastline runs along grid axes for five cells or more.

## What to do next, in order

1. Get `testLandscapePlane` green without touching the test; then finish Local and Site with the Region mechanics and steer drainage by the rock-mass field.
2. Cubes: sun shadows and snow on the WebGPU path, then the flip (WebGPU only, WebGL path deleted), then clouds and precipitation; in the engine, risers on gentle slopes carry the cover's material and readiness becomes truthful.
3. Engine: crew size by cores with the frame protected by thread priority, truthful readiness per level, the player on the finest level, water levels as one surface per body, no absolute positions as 32-bit floats.
4. Wire what is finished but not connected: interventions into the game, snow ladder into the frame and renderer, groundwater's real springs and caves into the voxel answer.
5. Re-measure against Earth and re-scout the catalogue after the relief rework and after each new process lands.
6. Produce what the brief asks for at delivery (below), have an independent reviewer go over code and architecture, write the honest evaluation.

## Delivery conditions of the brief not yet met

In-game captures with seed, date, coordinates and scale (coast with lowland, a range with a saddle, a river valley, a sandstone area with volumetric form, a mountain basin; from the ground and from the air; the three positions of the reference's known defects); scale and regularity figures including the tilted plane; the history table; intervention ledgers shown in the game (ocean to the void, two-lake cascade, stream diversion with a 100-year jump); a volumetric check (fly under a natural rock roof; air, roof and surface in one column in collision, voxels and simulation); frame-time percentiles in fast flight; cloud captures; an honest evaluation including a flight over ordinary country. Simulation-side numbers exist for most of these in the measure stream; nothing has been shown in the game on final data.

## How the work is run

- One working tree, many streams. Each stream owns directories; in files it does not own it makes additive, exact-match edits only, re-reads right before editing, and reports every line it added. Contracts between streams are agreed through the lead.
- A tile is a pure function of (seed, age, key); nothing may depend on request order, thread count or what was visited. Ledgers close. `sim::Workers::parallelFor` is safe inside a tile task; results must be bit-identical for any thread count.
- Places are found by search (`findLanding`, `terra find`, `terra scout`, `--at landing|coast|peak|shore`), never by fixed coordinates: the planet changed several times.
- The machine is shared and was heavily loaded (load 40–75); timings taken then are not baselines. Close every game instance you start; never kill a process you did not start.
- Per `AGENTS.md`, substantial changes get an independent reviewer; several streams list late fixes that were not re-reviewed.

## Quick reference

```bash
mise -C @simulation run build        # library, terra, tests
mise -C @simulation run test         # stops at the first failing check
mise run check                       # root: every module's checks
.out/simulation/build/macosx/arm64/release/terra planet --seed 42 --age 4.6e9 --out .out/terra/planet
.out/simulation/build/macosx/arm64/release/terra map --level 1 --at landing --tiles 2 --out .out/terra/landing
.out/simulation/build/macosx/arm64/release/terra relief ...   # slope and relief statistics (relief stream)
.out/simulation/build/macosx/arm64/release/terra view ...     # oblique picture from simulation data (relief stream)
.out/simulation/build/macosx/arm64/release/terra measure earth|scale|plane|seams|levels|determinism ...
DERIVEAN_CAPTURE=@game/capture/look.json @game/node_modules/.bin/electron @game   # after `npm run build` in @game
```

`terra planet` prints the land's hypsometry and the rock at the surface next to Earth's figures; use it after any change to the planet.
