# Hand-over: measurement, inspector and scouting (`@simulation/src/measure`, `@simulation/src/inspect`)

Describes the present. Rewrite as things change; history belongs in Git.

## 1. Goal

Turn the owner's acceptance criteria into numbers a lead can quote, and keep them runnable as the model changes:

1. Measure the generated world against the brief (scale, regularity, tilted plane, determinism, seams, level consistency, history, budgets, performance) and against the whole-Earth rows of [reference-landscapes.md](../reference-landscapes.md).
2. The inspector record: what the model knows about one point and why its surface material is there.
3. Scouting: find showcase sites and one site per catalogue type by search, never by fixed coordinates.

This stream does not change the model (`landscape`, `planet`, `strata`, `relief`, …). A defect is reported with numbers and a reproduction command.

## 2. State now

Code: `src/measure/**` (measurement functions, scout detectors, catalogue windows, Earth comparison), `src/inspect/**` (record, provenance, map layers), `tool/Measure*.cpp`, `tool/Measure.hpp`, `tool/InspectCommand.cpp`, tests `test/Landscape*.cpp`, `test/LandscapeFixture.hpp`, `test/Inspect.cpp`, `test/PlanetFuture.cpp`. Registrations only in `tool/main.cpp`, `tool/Arguments.hpp`, `test/Main.cpp`; a three-line `paintLayer` hook in `tool/MapCommand.cpp`.

### Commands

Place commands take `--seed --age` and a place: `--x --z`, or `--at shore` (`findShore`), `coast` (nearest Region shore cell to it), `peak` (highest point within `--reach`, read through `planetPoint`), `landing` (`findLanding`). They read a `--tiles` × `--tiles` block (default 3) of each level in `--levels` (default `123`).

| Command | Measures | Calibrated thresholds |
| --- | --- | --- |
| `terra measure scale` | Dry-land height above sea and above the first water a cell drains to (percentiles, class shares); relief, total climb and mean slope along lines of 0.25–64 km | Relief yardsticks per line length L km: plain 15·L^0.65, hills 150·L^0.43, mountains 1500·L^0.37 m (rounded from general knowledge, not sourced). Flags: median land above 1500 m; relief above the mountain yardstick |
| `terra measure regularity` | Valley-segment orientation rose; gullies along contour-parallel lines (autocorrelation, trough spacing, cell roughness, notches); crest spacing along straight lines; straight-shore share | Grid excess > 1.3 at 3σ (band ±7.5°, chord 32 cells, 16 when fewer than 1000 segments). Repeating profile: strength ≥ 0.6, ≥ 4 troughs, spacing scatter ≤ 0.25; grid scale if spacing ≤ 4 cells. Notch = cut ≥ 0.25 cell deep within 2 cells both sides. Saw-tooth: crest spacing scatter ≤ 0.2 and height scatter ≤ 0.35. Straight shore flag > 35 % |
| `terra measure references` | The regularity indicators on hand-made surfaces | Fractal relief: 4–7 % repeating, 0 % grid scale, roughness p95 ≤ 0.11. Gullies every 12 cells: 100 % repeating; every 3 cells: 83 % grid scale, roughness 1.0. Cone with fall line kept: grid excess 1.09; locked to eight directions: 2.51. Fractal coast 18.3 % straight shore; same coast in 8-cell blocks 100 % |
| `terra measure plane` | A hand-built inclined plane of one rock (`--slope`, default 3 %, sandstone, 3 cm roughness) at 0°, 22.5°, 45°, plain, with a rounded swale, with a crease: gully depth, notch spacing, roughness, heading error of gathered water, share of water gathered on the axis, axis depth | Test limits below |
| `terra measure conservation` | `shapeSlopes` alone on closed hand-built ground: does rock + loose stay constant | Any change is a defect |
| `terra measure determinism` | Planet digest and every tile in the chain bit for bit: all threads vs one worker, request order reversed, fresh world, replay; one `sampleBlock` across a ground-tile edge; site-level `surveyAt` at a tile edge before/after the neighbour exists | Bit-identical |
| `terra measure seams` | Height step and bend at tile edges vs inside; lake density within 3 cells of edges vs elsewhere; river discharge kept across edges, split by which side was solved first | Ratios, 1 = invisible. A river is "lost" when the wettest cell within ±2 of its target carries under 0.7 of it |
| `terra measure levels` | Child mean over each parent cell minus the parent; parent rivers found in the child within 1 and 2 parent cells (≥ half the discharge); standing-water agreement | — |
| `terra measure budget` | Per tile: change of mean ground = uplifted − eroded + deposited + residual; dissolved reported apart; ledger vs per-cell totals | Test limits below |
| `terra measure site` | The five above in one run | — |
| `terra measure history` | One coordinate at 0, 10, 100, 500 Myr, 1, 2, 4.6 Gyr: planet height, sea, crust, basement, plate and motion, contact, surface rock; Region ground/water/bed | — |
| `terra measure future` | Planet summary at chosen ages incl. straight-shore share and the planet ledger | Test limits below |
| `terra measure earth` | Planet against the whole-Earth rows: hypsometry, surface rock, dryland, inland drainage; from `--tiles` first-pass Region tiles: relief > 200 m per 5 km, Meybeck classes, roughness bins, lake share | — |
| `terra measure performance` | Planet time by age; first Site tile; build vs steps per level on one thread; tiles/s on all threads; memory | — |
| `terra measure fingerprint [--age 1.2e9\|4.6e9] [--threads n] [--time 0\|1] [--repeat n] [--blocks 0\|1]` | Seed 42: builds a fixed set of Local/Site/Ground targets (1.2 Gyr: the test focus, a second-pass Ground tile, `findShore`; 4.6 Gyr: the glaciated Site wall, coast Local 705 261, a river mouth) and hashes every field of each and of its dependency closure (heights, cover, water, detail, shore, volcanic, sheet, aeolian, ledgers); `--time 1` replays each target alone on one thread `--repeat` times and prints the least wall, user and sys (each its own minimum); `--blocks 1` hashes and times a 32 × 64 × 32 `sampleBlock` at each Ground target | A speed-up is bit for bit when every `tile` line and the `all` hash are unchanged |
| `terra scout` | Showcase sites and catalogue types around a start (default `findLanding`), with camera poses and a relief picture per site | Recipes in section 6 |
| `terra inspect` | The inspector record as key/value lines | — |
| `terra map --layer` | `cover`, `discharge`, `erosion`, `deposition`, `uplift`, `rain`, `plates`, `crust`, `crust-age` (the lead's `rock` stays in `MapCommand.cpp`) | — |

River thresholds used throughout are chosen, not derived: Region 1e8, Local 2e6, Site 6e4 m³/yr.

### Tests

All share one fixture world (seed 42, 1.2 Gyr) and a focus on high ground whose tiles are first-pass at every level; the 8 Gyr planet for the far-future check evolves on its own thread, started by the runner when `testPlanetFuture` is chosen (not by the fixture, so a filtered run such as `simulation-tests chart` does not wait for it at exit).

Determinism, what each check compares bit for bit. `testLandscapeWorld` cuts its voxel block in the fixture world only (`checkDeterminism(..., freshBlock = false)`): in the fresh one-thread world that block needs the focus's ground water and so the 3 × 3 Local and Region tiles around it, built alone, about 21 minutes. `terra measure determinism` cuts it in both worlds; both now build through the graph (`surveyAt` → `requireOver` → `buildAll`), so it compares graph against graph. Not compared anywhere: one whole independent world against another down to voxels; each product is compared instead, over finished inputs where named.

| Product | Thread count | Request order | Check |
| --- | --- | --- | --- |
| Planet | fresh world on 1 worker vs the fixture's pool | — | `testLandscapeWorld` |
| Region, Local, Site terrain (the focus chain, the ordered pass-1 Site tile and everything they wait for) | fresh world on 1 worker vs pool | fresh world asks last first | `testLandscapeWorld`; replay of the Site tile against the same neighbours |
| Region and Local terrain of pass 3 (waits for all eight neighbours) | `Setting::workers` null vs pool, over the world's finished neighbours | — | `testLandscapeWorld` (`checkLastPass`) |
| Regional ground water | `Setting::workers` null vs pool, focus Region tile | — | `testGroundwaterRegion` |
| Local ground water | `Setting::workers` null vs pool, focus Local tile and its east neighbour edge; three threads asking at once | — | `testGroundwaterTile` |
| Ground terrain with springs | `Setting::workers` null vs pool | neighbours made in the opposite order | `testGroundTile` |
| Ground terrain, build path | Demand vs graph, 1 and 4 workers (synthetic Site and aquifer) | — | `testBuildGraphTile` |
| Voxels | — | one point through every block that holds it | `testGroundTile` (`throughBlock`) |
| Weather | 2 vs 5 workers, one jump vs quarter days | — | `testAtmosphere` (`testDeterminism`); plume 2 vs 3 workers in `testPlume` |
| Live state after interventions | fresh world on 1 worker vs pool | — | `testIntervention` (`InterventionReplay.cpp`) |
| Every field of a few tiles against stored prints (`measure/fn/Fingerprint.hpp`): young world Region, Local, Site and the pass-2 Ground tile at the focus; present world coast Local 705 261, glaciated Site 5474 9868 and the Region tiles under them | — | — | `testFingerprintWorld` (world group), `testFingerprintPresent` (present group). Prints per platform in `test/StoredPrints.hpp`, since macOS and Windows libm differ. A change claimed bit for bit leaves them; a change meant to move the world replaces them with the lines the failing test prints, in the same commit |

| Test | Holds | Status |
| --- | --- | --- |
| `testLandscapeRegularity` | The reference readings above stay on their side of each threshold | passed on the tree of 26 Nov 01:14 |
| `testLandscapePlane` | Site plane: gully p95 ≤ 0.25 cell (1 m), nothing at grid scale, mean within 0.5 m. Local plane at 22.5°: heading error < 15°, grid-scale share ≤ 5 %. Crease gathers water (gathered > plane + 0.2, axis share > 21 %) | **fails** on the tree of 26 Nov 01:14: site plane gully p95 10.7 m. Passed on the planet-frozen tree of 25 Nov 23:38 |
| `testLandscapeWorld` | Determinism as above (no fresh voxel block, see the table); ledger equals per-cell totals at every level; Region residual within [−dissolved − 5 %, +5 %] of turnover | last run 25 Nov 23:38: passed |
| `testInspect` | Provenance classifies eight hand-built cases; record equals ground-level `surveyAt`; every listed column run equals `materialAt` | last run 25 Nov 23:38: passed |
| `testPlanetFuture` | At 8 Gyr: highest < 12 km, 1-in-100 land < 8 km, thickest crust < 120 km, mean crust < 24.75 km, land 3–60 %, deepest > −9000 m, eroded = deposited within 1 % | last run 25 Nov 23:38: passed |

At the time of writing (26 Nov 02:08) the tree does not compile: `volcanic/type/Vent.hpp` and `groundwater/type/Cave.hpp` both define `Conduit`. Nothing here could be re-run on it. `mise -C @simulation run test` last failed in `testChart` (another stream), root `mise run check` in `engine-tests` (another stream).

## 3. In progress

Nothing is running. No background jobs, no uncommitted experiments outside the files listed above.

## 4. Next steps

1. When the tree compiles: run the five tests, then `terra measure plane --levels 23` — it is the safety net for the relief rework.
2. Re-measure everything in section 6 on the current planet (it changed after those tables: carbonate from solute, larger plutons, seed 42 sea level 54 m). Local and Site figures also need repeating after the relief rework.
3. Scout seeds 7 and 1234 (`terra scout --seed 7`); only seed 42 was scouted.
4. Detectors not written: glacial troughs (A2), cirques (A3), fault-block range fronts (A4), tors (B3), mesas (C2), cuestas (C3), V-gorges with a head step (C5; the gorge detector found none), frost ground (F4). Deserts (I2, I3) need a run without the fixed rain.
5. Re-run the catalogue scouting after the process streams land (coast, aeolian, volcanic, icesheet, karst): the "process absent" list in section 6 is what should shrink. Each new process needs its recipe added to `measure/fn/Catalogue.cpp` or a detector of its own.
6. Unreviewed code (written after the second independent review): `measure/fn/Catalogue.*`, `measure/fn/Earth.*`, `tool/MeasureEarth.cpp`, `straightShoreShare` in `measure/fn/Regularity.cpp`, the rock-weighted bed thickness and the run-base dip in `measure/fn/ScoutRock.cpp` / `ScoutCity.cpp`, the edge probe in `measure/fn/Determinism.cpp`, the switch to live plates in `inspect/fn/Crust.cpp` and `Layer.cpp`. The bed-thickness change has not been run on real ground.
7. Performance on an idle machine with the Ground level: not measured.
8. `docs/simulation.md` does not mention these commands.

## 5. Decisions and why

- **`DERIVEAN_RAIN=0.9`.** An environment variable read in `landscape/fn/Forcing.cpp` replaces the planet's precipitation with 0.9 m/yr for every landscape tile. All tile-level figures here used it, so that they did not swing while the climate code was rewritten. Consequences: no dry ground exists at tile level (desert types cannot be found), and rain falls on the pre-ocean planet in `history`. Planet-level figures (hypsometry, dryland share, ledger) are unaffected.
- **Places by search.** Fixed coordinates went stale every time the planet changed; the lead's original three were sea within hours. Landscapes read the planet through `planetPoint` (a seeded offset of up to a planet cell), so a place found on the raw planet grid is not where the landscape puts it; `peak` and the test focus therefore ask heights through `planetPoint`.
- **Thresholds come from references, not taste.** Each regularity indicator is read on surfaces with a known answer (`terra measure references`, asserted in `testLandscapeRegularity`).
- **Tests assert only invariants that hold.** Where the model violates a criterion the number is reported and the test asserts the nearest thing that holds. The plane test's site-level limit is one of those and now catches the relief rework.
- **Budget = uplift − erosion + deposition + residual.** `TileLedger::dissolved` mixes river solute (already inside `eroded`) with weathering solute, so it cannot be subtracted; the Region test bounds the residual by it instead.
- **Synthetic plane through the real `stepDomain`.** `measure/fn/PlaneDomain.cpp` hand-allocates a `Domain` because the model's `allocate` is private. It must list every per-cell array of `Domain`; it has broken twice when arrays were added (`iceAge`, `quality`). Exporting `allocate` from `landscape/fn/Build.cpp` would end that.
- **Scout is coarse to fine.** Region tiles within the radius, each detector names candidates there, Local tiles are built only around candidates, Site and Ground tiles only for sampled rock blocks. Detectors take metres, so one detector reads both a Region and a Local mosaic.
- **Inspector reads the ground level** (`surveyAt(…, Level::Ground)`), the surface voxels are cut from; slope and "steeper ground above" are taken over site cells.

## 6. Results that must survive

**Planet state A** for every table below unless marked: seed 42, 4.6 Gyr, sea level −6 m, land 22.6 %, landing (−2581902, −4571906); `DERIVEAN_RAIN=0.9`; measured 25–26 Nov. The planet has changed since: treat coordinates as stale and numbers as a baseline.

### Earth comparison (`terra measure earth`, 64 first-pass Region tiles, 550 000 km² of land)

| Quantity | Ours | Earth |
| --- | --- | --- |
| Land share | 22.6 % | 29 % |
| Land < 200 / 200–500 / 500–1000 / 1000–2000 / 2000–3000 / 3000–4000 / above | 29.1 / 25.1 / 17.7 / 17.5 / 5.7 / 3.7 / 1.2 % | 29 / 28 / 24 / 13 / 3 / 1 / 2 % |
| Land height: mean; quartiles | 803 m; 168, 430, 1121 m | 673 m; 161, 408, 846 m |
| Rock at the top: sedimentary (carbonate) / metamorphic / plutonic / volcanic | 85.4 (2.4) / 11.5 / 1.2 / 2.0 % | 64 (≈ 21) / 13 / 7 / 6 % |
| Dryland (rain < 65 % of the demand `runoffOf` uses; planet's own climate) | 27.1 % | 41.3 % |
| Land draining to no ocean (steepest way on the 65 km grid) | 6.0 % | 13 % |
| Shore cells in axis-aligned runs ≥ 5 | 37.0 % | fractal coast 18.3 % |
| Land with > 200 m relief per 5 km | 34.4 % | 12 % |
| Meybeck plains / lowlands / hills / plateaus / mountains | 42 / 11 / 18 / 1 / 28 % | 25 / 14 / 23 / 13 / 25 % |
| Roughness < 5 / 5–10 / 10–20 / 20–40 / 40–80 / 80–160 / > 160 m/km | 42 / 8 / 10 / 14 / 8 / 11 / 7 % | 25 / 19.5 / 17.8 / 16.8 / 14.7 / 5.8 / 0.4 % |
| Standing fresh water, share of land cells | 15.6 % | not in the catalogue |

### Headline site figures (state A)

- Landing, Region 786 km: land above sea p50 2915, p95 6477, max 9696 m (planet's highest cell 5344 m); 16 km relief p50 1168, max 4416 m. Local 49 km: 1 km relief p50 187, p95 619 m; mean slope along lines 50–64 %. Seed 7 peak, Region: p50 4280, max 9949 m.
- Seams, Region: lakes 1.9–4.1× denser within 3 cells of edges; 30–57 % of rivers lose > 30 % at an edge (41–93 % into a tile solved earlier, 12–34 % into a later one).
- Levels: Local mean minus Region rms 177–245 m at mountain sites (parent cell step 243–363 m); parent rivers found within one parent cell 68–88 %, 8 % at seed 7 peak.
- Site-level valley segments: axis excess 1.9–2.3 at 3.1–4.4σ on five of five sites.
- Far future: highest 5344 / 4104 / 6094 m, thickest crust 68 / 67 / 86 km, land 22.6 / 32.0 / 37.9 %, mean crust 11.9 / 19.2 / 25.6 km at 4.6 / 8 / 12 Gyr.
- Determinism (landing, pass-3 Region order test): planet and 19 tiles bit-identical. Exception measured once at 1.2 Gyr: site-level `surveyAt` within 4 m of a site-tile edge moved by mean 0.86 m, worst 3.19 m once the neighbour existed.
- Plane, Local, 3 % sandstone (planet-independent; model of 25 Nov 23:42): gully p95 77 / 97 / 125 m at 0° / 22.5° / 45°, troughs 7–8 cells apart, heading error 4–6°, crease gathers 53–81 % of the water without cutting deeper than its flanks. Site plane flat (p95 ≤ 1 m). On the relief-rework tree of 26 Nov 00:36: Site p95 6–8 m; Local p95 284–374 m with a notch every 3 cells and roughness 1.00 at 0° and 45°.
- Performance (tree before the Ground level, idle machine): planet 6.85 s to 4.6 Gyr; first Site tile 24.9 s; 6.3 / 4.4 / 2.6 s per Region / Local / Site tile on one thread; 0.7–1.45 tiles/s on 15 threads; 6.2 MB kept per tile.

### Defects, ranked, with reproduction

1. Local dissects any slope into parallel cell-wide gullies; worse on the relief-rework tree. `terra measure plane --levels 2`
2. Region and Local relief far beyond Earth around mountains; Region exceeds the planet by ~4 km. `terra measure earth`; `terra measure scale --at landing`
3. Lakes on 15.6 % of land, piled at tile edges, hundreds of metres deep at Local. `terra measure seams --at landing --levels 1`
4. Rivers lose their water at Region tile edges, worst into tiles solved earlier. Same command.
5. Site-level valleys follow the grid axes. `terra measure regularity --at peak --reach 3000000 --levels 3`
6. Local does not reproduce its parent. `terra measure levels --seed 7 --at peak --reach 3000000 --levels 2`
7. Site-level `surveyAt` (the default level) depends on which neighbour tiles exist. `terra measure determinism --at landing` prints the edge probe when the neighbour is not yet built.
8. The crust stock grows without bound (~0.5 × 10⁹ km³ per Gyr net). `terra measure future --ages 4.6e9,8e9,12e9`
9. Planet coastline: 37 % of shore cells in axis-aligned runs. `terra measure future --ages 4.6e9`
10. The tile ledger cannot close below Region; slope transfers to fixed cells are unbooked (dunes lose 4.2 % of their sand per Site step). `terra measure budget --at landing`; `terra measure conservation`
11. Steep bare sandstone or conglomerate on 9.9 % of dry Local cells. `terra scout`

### Scout sites: how each kind is found

Driver (`measure/fn/ScoutAround.cpp`): Region mosaic within 250 km of the start; up to five Region candidates per kind, nearer ones preferred; each confirmed by the same detector on Local tiles around it; where none confirms, the Region candidate is listed as "region level only". Lakes, cliffs, gorges, rock cities, cold summits and catalogue windows are also collected from every Local mosaic the search builds.

| Kind | Recipe | Found on state A |
| --- | --- | --- |
| a coast with lowland | shore cell; 90 % of land within 5 km below 150 m (3 %); land fills 30–80 % of that disc | 2 sites ~540 km from the landing, land within 5 km under 5–6 m |
| b saddle | along one of four axes a summit ≥ 300 m higher within 10 km on both sides, pass lowest between them; ground falls ≥ 60 m within 4 km across | passes at 4868 m and 3017 m |
| c river valley | cell with ≥ 1e8 m³/yr; walls ≥ 150 m on both sides within 3 km | 18.3 m³/s between walls of 1167 and 1344 m |
| d rock-city ground | Sandstone at the surface; one run ≥ 60 m thick within 100 m of the rock surface; dip < 3° from the run's base 100 m east and south; relief 80–300 m within 1 km | 2 sites, 93–98 m of sandstone, dip 0.2–0.4°; voxels show 0.01–0.8 % roofed columns |
| e mountain lake | lake ≥ 0.05 km², level ≥ 800 m, ≥ 3e5 m³/yr leaving over dry ground whose flow does not return, ground rising ≥ 100 m within 1.5 km | 2–3 km² lakes at 3874 m and 3974 m, 330–350 m deep |
| f cold crystalline summit | Granite or Gneiss, annual mean ≤ 0 °C, highest within 1 km, ≥ 600 m over the lowest ground within 3 km; reports ice now and at the glacial maximum | Gneiss summits at 8228 m and 6598 m, ~200 m of ice |
| g hot ground | `geothermAt(planet, planetPoint(...)).gradient > 0.15` K/m on dry ground with ≥ 0.3 m/yr of rain | 177–188 K/km, 88 % from melt 2.6–4.0 Myr old |
| h steep bare sandstone | Sandstone or Conglomerate, cover < 1 m, slope ≥ 35 %, ranked towards 45° faces on level beds; a 96 m block sampled at 0.5 m for rock over air | 5–13 % of rock columns roofed, gaps to 30 m, 47–179 forms, nearest-neighbour scatter 0.49–0.60 (random placement gives 0.52) |
| i gorge | cell with ≥ 3e6 m³/yr; both rims ≥ 100 m above it within 0.5–1 km and that ground level within 35 m | none |
| j ordinary country | 10 km line, all dry and ≥ 15 m above the sea, relief < 150 m, no step over 70 % | Region level only; not confirmed at 64 m |

### Catalogue types (state A, 6.19 M dry Local cells around the landing)

Window recipes live in `measure/fn/Catalogue.cpp` (5 km windows on Local mosaics; criteria no tile field carries are left out).

- **Found:** A1 collisional high range, B2 granite domes, C1 rock city (list d), C6 badlands (rain criterion not applied), E5 hot ground (list g), F1 lake country (at 5650 m), G2 floodplain, G3 coastal lowland (list a), J3 rolling upland.
- **Found but doubtful:** F2 fjords and H1 cliffed coast both matched one steep mudstone coast; no trough, and nothing cuts platforms or stacks.
- **No site, recipe never met in one window:** B1 old worn mountains, C4 canyon lands, J2 open basins.
- **No site, process absent:** D1–D3 karst (limestone is only lowered evenly by solution), E1–E4 volcanic edifices (eruption thickens a planet cell and lays a bed), F3 drumlins and eskers, G1 braided rivers (a river is one path of cells), G4 lake beaches, H2 sandy coasts and spits, I1 dunes, J1 loess.
- **Not searched, no detector:** A2, A3 (nearest: lists e and f), A4, B3, C2, C3, C5, F4.
- **Not searched because rain was fixed:** I2, I3.

## 7. Build, run, verify

```
mise -C @simulation run build          # library, terra, simulation-tests
mise -C @simulation run test           # all streams' checks
mise run check                         # worktree root: @game type check and engine-tests
```

Binary: `.out/simulation/build/macosx/arm64/release/terra`. Typical runs:

```
DERIVEAN_RAIN=0.9 terra measure site --seed 42 --age 4.6e9 --at landing --out .out/measure
DERIVEAN_RAIN=0.9 terra measure earth --seed 42 --age 4.6e9 --tiles 64
DERIVEAN_RAIN=0.9 terra measure plane --levels 23 --out .out/measure
DERIVEAN_RAIN=0.9 terra scout --seed 42 --age 4.6e9 --keep 2 --out .out/scout
DERIVEAN_RAIN=0.9 terra inspect --seed 42 --age 4.6e9 --at landing
terra measure references
```

Costs on an idle 16-core machine before the Ground level: a `site` run about a minute, a scout about five; under load they took 4–10 and 65 minutes. Rock-block sampling in the scout is the slow part (Ground tiles); `--keep` sets how many sites per kind and so how many blocks.

Verification of a change here: the five tests, `terra measure references` unchanged, and an independent reviewer pass (AGENTS.md). When another stream's half-edit breaks the build, wait and retry; do not work around it.

## 8. Open questions

- Should `testLandscapePlane` also bound the Local plane (gully depth, notch spacing)? It does not, because the frozen model already failed any sensible limit there; after the relief rework settles a limit should be chosen and asserted.
- Is the site-level `surveyAt` edge dependence acceptable, given that voxels are cut from the ground level? The default level of `surveyAt` and `surfaceAt` is still Site.
- Is unbounded crust growth intended? The far-future test only bounds mean crust at 8 Gyr.
- The Earth figures for line relief in `terra measure scale` are unsourced yardsticks; the catalogue's Meybeck rows are the sourced comparison.
- The endorheic share is read on the 65 km planet grid by steepest descent; a Region-level reading would be more faithful and far more expensive.

What the inspector cannot explain yet:

- Cover moved by creep or slope failure: tiles keep no per-cell record of it, so thick cover with no recorded deposit falls to "lake/sea floor", "slope wash" or "not explained".
- Glacial till as an origin of its own (`Tile::iceAge` exists; provenance does not use it).
- Veins and hot springs: `InspectVeins` and `InspectHeat` are fields only; provenance has no branch that names them as the cause of a surface material.
- Weather and clouds: `InspectAir` holds annual means; the place for the atmosphere's per-point weather is there, unfilled.
- Why the erosion model's bed (`modelBed`) and the voxel column's first rock differ where they do; both are shown, the difference is not explained.
