# Engine hand-over

Owner of this file: the engine stream (`@engine/**`, `@napi/**`, `@game/contract/GameBridge.ts`, `@game/main/engine-worker.cjs`, `@game/main/engine.mjs`). It describes the present. Paths are relative to the repository root. Nothing of this stream is committed; the lead commits.

## 1. Goal

The playable world on one simulated planet: voxels streamed from `@simulation` around the player without the frame ever waiting, the far view out to 260 km drawn as the same world at coarser cube sizes ("cubes all the way"), water, weather and clock passed through unchanged, and a bridge contract the renderer can draw with one pipeline. Fill by what the eye gains, on every core, with a truthful readiness signal.

## 2. State now

Written 2026-11-26 02:30 local.

Works, last verified:
- Native tests up to the far-patch checks: `engine-tests` ran green through `testWorld`, `testScenarios`, `testStreamedWorld`, `testWeather`, planet spawn (01:48 build).
- Thread bench at age 2e9, far patches, 01:48 build (before own-place tile priority), quick mode. The machine's load average was about 30 during the first run and rose to 85 during the second, so the two are not a fair pair:

  | | crew 5, frame thread plain | crew 12, frame thread favoured |
  | --- | --- | --- |
  | voxels to stand on after start | 18.1 s | 22.3 s |
  | walking step mean / p95 / p99 / max | 0.43 / 1.52 / 5.81 / 16.9 ms | 0.34 / 1.35 / 4.08 / 6.6 ms |
  | voxels whole after a 1.5 km teleport | 259.7 s | 615.3 s (load 85) |
  | step while waiting there | 0.22 / 0.76 / 1.84 / 12.2 ms | 0.17 / 0.55 / 1.37 / 39.7 ms |
  | boost flight of 103 km, step | 0.57 / 1.52 / 3.49 / 11.7 ms | 0.64 / 2.98 / 6.54 / 16.6 ms |

  First far surface in the first frame and the far view whole after 1.5–1.9 s in both.

Broken or unverified right now (03:00):
- The whole tree compiles (02:48 build: engine, tests, bench, addon). `engine-tests` passes `testWorld`, `testScenarios`, `testStreamedWorld`, `testWeather` and then stops in the planet checks; **`testCubes` has never run to its end**, so the cube builder and the levels are covered only by the renderer drawing them and by clang.
- The simulation's landing at seed 42, age 2e9 now lies on a lake 871 m deep (`sim::findLanding`; it was dry ground an hour earlier). Every planet check assumed dry land ("A player without voxels around must wait" failed). `standOnLand` in `@engine/test/Planet.cpp` now moves the player to the nearest dry ground first; the run with it is in flight. The bridge tests in `@napi/test/session.test.cjs` make the same assumption and have no such step yet.
- "A far patch shows water that is not a level sheet" (patch path) fails in some runs and not in others, depending on which tiles are finished; `checkPatch` prints the neighbourhood when it does. The patch path is to be deleted, so this was not chased.
- `farColoursMatchVoxels` (patch path) has not been reached since the cover rule was updated.
- The simulation's headers collide: see `@engine/src/nature/type/Air.hpp`, which renames `Sand` of the ash classes with a macro so that the engine compiles. Delete the macro once `@simulation/src/atmosphere/type/Plume.hpp` scopes its enum.
- Bridge tests and `mise run check` have not completed since the cube work started (`chart.test.cjs` passed at 02:40).
- Not measured: the bench in a world of cubes (faces and milliseconds per level), and anything after the own-place tile priority and the wider tile reaches went in.

## 3. In progress

Taken over 2026-11-26 08:50 by a new worker. Edits since (all compile, 09:30); an independent review was applied (its points 1–5 and the minor ones):
- The planet checks run in a world of cubes: `@engine/test/PlanetSession.hpp` holds cube chunks (`Held`, `hold`, `checkBoxes`, shared with `Cubes.cpp`) instead of patches; `farColoursMatchVoxels` became `cubesMatchVoxels` (finest cube tops against the top voxel); `becomesReady` waits for every readiness signal at rest and checks that settled columns read what a build now would. `checkPatch` went (its water check failed at random on the patch path, which is to be deleted). `Cubes.cpp` gained `risersCarryCover`. `Main.cpp` unbuffers `std::cout` so a log shows progress.
- `findSpawn` (`@engine/src/game/fn/Spawn.cpp`) puts a player without a wished place on the first dry ground round the simulation's landing when the landing lies under water: rings of eight doubling from 250 m, blocking surveys local → site (3 m margin) → ground, so the place is a function of the world. `standOnLand` in the test is gone.
- Readiness (`@engine/src/horizon/fn/FarCubes.cpp`): a level's `wanted`/`held`/`settled` count only columns reaching into its box at full reach plus band (`sightOf`); the margin column is built but holds nothing back. `settled` needs `cubeFinal(detail)`: the four corners of the square a column reads are at its source level, so nothing in it is eased towards stand-in data. In a world of cubes `TileRequests::reach` and `::ceiling` come from `cubeDataReach`/`cubeCeiling` (default reach: region ≈ 321 km, local ≈ 45 km, site ≈ 2.8 km, ground 96 m; ceilings local 41 km, site 2.56 km, ground 150 m), so `ready.{region,local,site}` wait for exactly the tiles the levels read. The request square spans both the eye and the point ahead of it.
- `engine-bench ... ready [seconds]` (`@engine/bench/Ready.{hpp,cpp}`) prints the readiness signals and what holds each of them.
Running (09:30): `engine-tests` (`.../scratchpad/engine/tests4.log`), bridge tests (`node1.log`).

Half-done things that look odd:
- Two far-terrain paths exist. `PlanetOptions::cubes` (bridge: `WorldOptions.cubes`) picks cubes (`Game::cubes`, `@engine/src/horizon/type/FarCubes.hpp`) or patches (`Game::horizon`). The patch path (`Patch.*`, `Horizon.*`, `Select.cpp`, shading raster, `farPatch` in `@napi/src/far.cpp`, `FarPatch` in the contract, `checkPatch`/`farColoursMatchVoxels` in the tests) is to be deleted once the renderer draws cubes over the whole view. Do not invest in it.
- The engine keeps faces as nine `int16` per quad everywhere (`FaceMesh`). The addon converts: packed quads for chunks in a world of cubes, eight words (occlusion stripped) for a world of patches.

## 4. Next steps

1. Get `engine-tests` green on the cube session (section 3), then the bridge tests (`cubes.test.cjs`, `session.test.cjs`, `chart.test.cjs`) and `mise run check`.
2. Captures: the renderer's `worldSettled` (`@game/src/session/fn/worldSettled.ts`) should add `frame.ready.far`, and its stall rule (`CaptureDriver.ts`, 8 s without arrivals) should count a change of `frame.tiles.*.finished` as an arrival: tile builds take tens of seconds each, so a capture is called stalled while the engine is still working. Asked through the lead; the files are the renderer's.
3. Delete the patch path and `WorldOptions.cubes` once the renderer flips to WebGPU (coordinate with the lead first): `Patch.*`, `Horizon.*`, `Select.cpp`, `farPatch`/`farPatches` in `@napi/src/far.cpp`, `FarPatch` and `Frame.far` in the contract, `session.test.cjs`'s `applyFar`, the fixed reaches in `TileRequests.hpp` (cube reaches become the only ones).
4. Bench a world of cubes: `engine-bench 2e9 cubes 0 favoured` for faces and milliseconds per level and boost-flight step percentiles; compare with `patches`. Only on a machine below load 40.
5. Eviction (needs the simulation's atlas): `evict` by level, farthest first, keep radii of twice `TileRequests::reach` per level, budget about 2 GB, never a tile a queued build depends on or a live (edited) tile. Trap: `sim::TileReader` returns raw `Tile*` from a cache of four `shared_ptr`s, so a pointer taken before four newer lookups can dangle once tiles may be dropped; readers must not hold a `Tile*` across lookups, or the reader must pin what it hands out. Evicting a tile under a shown cube column makes `cubeDetail` read coarser data and rebuild the column coarse, so keep radii must cover `cubeDataReach`.
6. Volume levels: only ground tiles hold overhangs, and they exist within about 100 m of the player, where the finest far level (0.5 m) is almost hidden by the voxel box. Volumetric far cubes (arches and overhangs seen from kilometres) need a volume answer from site data; ask the ground and relief streams whether `relief/type/Form.hpp` can give one. Not started.
7. Interventions (queued by the lead, not started): route the brush into `intervene` strokes, one per frame of brush work; drive `tick` from the game clock and `jumpTo` from the time controls; re-mesh chunks whose live tile changed; ledgers to the HUD; the atmosphere must read `world.ocean`. Needs a gate so no `sampleBlock`/`surfaceAt` reader runs during `intervene`/`advance`/`tick`/`jumpTo`/`syncLive`, without the frame waiting: background tasks hold a shared lock per task, the game thread takes the exclusive lock between frames with `try_lock` and a "pending" flag that makes new tasks wait. `EditStore` (per-chunk voxel overrides) then stops being the owner of dug ground.
8. Eruptions (advance notice): terrain changing under live tiles without a stroke. Blockers in the engine: no notification of changed tiles is consumed yet; `cubeDetail` signs data levels, not content, so a changed tile does not refresh far chunks (needs a revision stamp per tile from the simulation); voxel chunks are regenerated only on unload or edit; there is no heat and only one liquid (integer water units), so molten lava needs material properties for viscosity, solidus, heat capacity and what it freezes to, and a second liquid in `@engine/src/water`.
9. Snow: the ladder is passed through; nothing left in the engine.

## 5. Decisions and why

- **Cubes rule** (`@engine/src/horizon/type/CubeChunk.hpp`): a cube is solid if the world is solid at its centre. Rejected: mean height (shaves crests), "any solid" (fattens). The top cube shows the surface cover however thin; under water a column ends strictly below the level and the dry column beside it at or above it; a column counts as under water only if all eight neighbours know a level (else the carried level ran out and an open edge appeared).
- **Clipmap, not quadtree**: every level is a square of columns of chunks round the eye and is whole under the finer levels too. That costs a third more chunks and removes holes by construction: a missing or slow level just leaves the coarser one showing. Boxes per level are sent every frame and shrink to what is held.
- **Reach 128 cubes** (`FarCubes::reach`, `WorldOptions.cubeReach` 64–256): a cube is 1/128 rad at the side of its box, 5–7 px at 900–1350 px of height. 192 would be about 4 px but more than doubles the faces. Finest far level 0.5 m (level 2), coarsest 1024 m (level 13) stretched to 260 km instead of a 2 km level.
- **Chunks are 32 cubes on every axis** because the renderer's packed quad has 6-bit coordinates. A column of chunks is the unit of building and swapping; its water sheets ride on its highest chunk.
- **No volume in the far levels yet**; site and coarser data are height fields, so only ground tiles could add overhangs.
- **Riser materials** come from the planet's strata (`@engine/src/horizon/fn/Beds.cpp`), read at the column's own position; voxels blend strata on a 16 m lattice, so contacts can differ by less than a far cube.
- **`surfaceMaterial`** (`@engine/src/horizon/fn/Tint.cpp`) mirrors the simulation's cover rule from a surface sample. It cannot know frost-sorted or scalded ground, clay, or stones. The simulation should hand out the top material itself.
- **Standing water** (`StandingWater.cpp`): read at site level, a wet point needs a partner one data cell away at the same level, levels are carried at most five points over ground below them. `WaterRing = Shore + 1` is what makes two lattices agree on a shared point.
- **Occlusion** in the mesher: a quad grows along an axis only if its corner values do not change along it. Chunks therefore need all 26 neighbours for a final mesh (`meshChunk`, `markTerrainChanged`, `TerrainStream.cpp submit`, `Column.cpp mesh`).
- **Pools** (`Nature.hpp`): simulation pool (hardware − 1), crew = performance cores (`hw.perflevel0.physicalcpu`, else hardware − 2), air = 2. Two pools, not one: a pool task cannot be interrupted, so a chunk would wait behind a tile build of seconds. The thread that advances the game calls `favourCallingThread()` (macOS QoS user-interactive; nothing on Linux, unverified). Crew priorities: voxel columns and colliders 0, far cubes `level − 64`.
- **Tile requests** (`TileRequests.cpp`): by gain (replaced cell size over distance); the player's own place (within 150 m, when lower than 150 m above ground) first and with every worker, the rest leaves a quarter of the workers free. Requests go to `atlas.request` directly because `sim::requestAround` ranks coarse levels first.
- **Shutdown**: destroying a `Game` waits for every tile build in flight (the simulation destroys its atlas before joining its pool, so `~Nature` must quiesce it). `Session.abandon()` leaks the world for a process about to exit; `engine.mjs` loads the addon in the main thread too so it is not unloaded with the worker.
- Traps: `sim::surfaceAt` locks the atlas per sample, use `SurfaceReader`; readers that call `readSurface` must call `overlayLive`; a `SurfaceReader` must not outlive its batch; the simulation tree is often mid-edit, retry builds.

## 6. Contracts with other streams

`@game/contract/GameBridge.ts` holds all of it; the far-cube contract **is in the file**:
- `WorldOptions.cubes?`, `cubeReach?`; `WorldInfo.faceWords`, `WorldInfo.cubes { reach, side, band, finestLevel, coarsestLevel, viewDistance } | null`, `WorldInfo.snowLadder`.
- `Quads = Uint32Array`, two words per quad: word 0 plane (bits 0–5), lowU (6–11), lowV (12–17), sizeU − 1 (18–22), sizeV − 1 (23–27), axis (28–29), sign (30); word 1 material (0–7), occlusion (8–15, two bits per corner in the order low-low, high-low, low-high, high-high).
- `ChunkMesh { key, origin, faces: Int16Array, quads: Quads | null }`: in a world of cubes voxel chunks have empty `faces` and their quads in `quads`; water meshes and debris stay `Int16Array` with `faceWords` words.
- `CubeChunk { id, level, edge, origin, quads, water: { level, quads }[], dataLevel, wantedLevel }`; `CubeLevel { level, edge, wanted, shown, box }`; `Frame.cubes { added, removed, data, levels } | null`.
- Readiness: `Frame.ready.far` is true when every level holds all the squares of ground it wants (those reaching into its box at full reach and band), each read from the level's source all over (`cubeFinal`) and none waiting for a swap, and every finished tile has been looked at; `Frame.pending.patches` is the number of squares still short of that. It covers cube chunks. In a world of cubes `ready.{site,local,region}` are about the tiles the cube levels read, nothing else. Per level `Frame.cubes.levels[k].{wanted, shown, settled, box}`: drawable is `box`, final is `settled === wanted`. Tiles: `Frame.tiles.{region,local,site,ground}.{wanted,finished}` and `Frame.ready.{site,local,region}`. Tile reaches (region 330 km, site 3 km) were widened so that every column a level wants can reach its data level.
- Answers to the renderer's questions (2026-11-26): two sheets of one water level in a chunk no longer occur (levels closer than 2 cm are one sheet since `SheetGap`; distinct sheets have disjoint footprints); a level holds every chunk with a face over each square inside its box's footprint at any height (`checkBoxes` in the tests checks it every frame); a riser one cube high is the side of the top cube and so carries the top's material; taller risers on slopes up to 45° with cover carry the cover (`risersCarryCover` checks it).
- Risers: a step's side carries the surface cover where the ground is a slope of at most 45° with cover on it, the bed at its depth where steeper or bare. Culling: only faces between solid and air of one level; no bottoms; nothing under the lowest neighbouring top; chunks without a face are not sent. Chunks under a finer level's box are kept (they are what shows when the finer level rests) and can be skipped by the renderer with a box test.
- One water sheet per water body per column of chunks: levels closer than 2 cm are one sheet.
- `CloudField.snowLadder`, `snowLine`, `seaLevel`; `Frame.weather.snowpack` is snow at the player's height.
- `CloudField.aerosolDepth`, `ashAloft`, `ashLevel` are forwarded unchanged from the simulation's cloud field (zero until a source exists); the inspector shows `WeatherReport::ash` and `aerosolDepth`. The atmosphere's side is in `docs/handoff/atmosphere.md`.
- Nothing measured from the world origin is `float32`: particles are relative to `Frame.particleOrigin`, patch heights to `origin.y`, quads to their chunk.
- Shared files: `@napi/src/addon.cpp` (`abandon`, `faceWords`, `cubes`, `snowLadder`, `favourCallingThread`; the map stream owns the chart lines), `@game/main/engine-worker.cjs` (transfer list entries for `quads`, cube chunks, `snowLadder`, `snowLine`, `aerosolDepth`, `ashAloft`, `ashLevel`; message kind `close`), `@game/main/engine.mjs` (`close`, addon pinned).
- Asked of the simulation: rename `AshClass::Sand`; top material and cover kinds in `SurfaceSample` at every level; a non-blocking bed-at-depth query; a revision stamp per tile; eviction (section 4); destroy order of `World`.

## 7. How to build, run and verify

All from the repository root; xmake comes through mise.
- Build tests, bench, addon: `mise -C @napi exec -- xmake build --yes engine-tests`, `... engine-bench`, `mise -C @napi run build` (addon at `.out/addon/derivean.node`). A cold build takes minutes.
- Native tests: `.out/xmake/build/macosx/arm64/release/engine-tests`; silent on success, exit 0. The planet takes 1–3 minutes to evolve, the whole run 5–10 minutes on a busy machine.
- Bridge tests: `cd @napi && node --test test/*.test.cjs` (each file evolves its own planet).
- Types: `mise -C @game run check`; everything: `mise run check`.
- Bench: `.out/xmake/build/macosx/arm64/release/engine-bench [age] [patches|cubes] [crew threads, 0 = machine] [plain|favoured] [quick]`. It prints fill times, step percentiles for idle, walk, a 1.5 km teleport and a boost flight of 100 km, and per cube level chunks, faces and milliseconds.
- Capture (renderer's): `DERIVEAN_CAPTURE=<plan.json> mise run play` after `mise -C @game run build`.

## 8. Open questions

- Is 128 cubes of reach the look the owner wants, or 192 at more than twice the faces?
- May the finest far level start at 1 m instead of 0.5 m (the 0.5 m level is almost hidden by the voxel box of 52 m)?
- Who fixes the `Sand` collision, and can simulation headers that the engine includes be compiled together in the simulation's own check?
- Eviction budget and rule (section 4, item 6).
