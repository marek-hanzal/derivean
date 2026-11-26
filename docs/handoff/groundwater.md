# Hand-over: ground water, springs, caves, karst

Owner of `@simulation/src/groundwater/**`, `@simulation/test/Groundwater*.cpp`, `@simulation/tool/Groundwater*.cpp`. Nothing is committed.

## 1. Goal

The owner wants a world that is not empty and not a height map. Springs, cascades, caves, sinkholes and hot ground must arise in the simulation, never be placed. Water must come from somewhere (rain that infiltrated), and whatever a player finds must be explainable ("why is there a spring here?"). The world has no life. Karst is this stream's in full: caves and the surface forms (sinking streams, dolines, pavement, poljes, towers). The surface forms can only be delivered as hooks, because `landscape/**`, `ground/**`, `relief/**` and `inspect/**` belong to others.

## 2. State now

Builds: `mise -C @simulation run build` succeeded with everything below in it (library, `terra`, `simulation-tests`).

Works and how it was checked:

- Whole pipeline for a Local tile: columns from cover, volcanic pile and beds; recharge; sinking streams; levels; routing in two storeys; springs with heat and loads; conduit network; cave field; `caveSpans`/`inCave`; `springsIn` with site refinement; explanation records; per-world cache `World::groundwater`.
- `test/GroundwaterBlock.cpp` and `test/GroundwaterEdge.cpp` pass (3.3 s CPU): contact spring line (256 springs on 256 cells of outcrop, 99.9 % of the recharge), limestone block (sunk stream of 95 l/s rises 10.9 km away, 124 km of passages over 0.5 m, 97 % along joint zones, spans checked against the tubes), fault spring (98 °C, 17 % steam, silica with heat; 33 °C and nothing without), two tiles worked out apart agree to the bit along their shared edge and on the water crossing it.
- `test/GroundwaterTile.cpp` passed in an earlier form on the real young world (1 thread = pool bitwise, real shared edge bitwise, cache from three threads). Since then `checkVents` (springs of a rectangle = springs of its halves, `explainSpring`) and an upper bound on underflow were added and NOT yet run.
- `terra groundwater` ran at three searched places with an earlier build; numbers and maps were read (section 5, "what the runs showed").

Not verified:

- `test/GroundwaterTile.cpp` in its present form. On that tile (seed 42, age 1.2e9, `testFocus`) underflow was 65 % of recharge with the earlier build; the test now fails above 50 %. Cause not found yet: `terra groundwater --time 1` prints where the lag sits (on tight beds / fault rock / under tight beds / elsewhere).
- Timing of the real-tile test inside the suite: it needs 11 more Local tiles than the other tests build (about 70 s CPU, under 10 s on an idle 16-core machine; 16 minutes when the machine ran at load 110).
- Volcanic piles, the three-piece transmissivity profile, the karst underflow at tile edges and the reworked heat were written after the last real runs.
- Voxel look at a hot spring and a cave mouth (`terra slice`, `terra ground`): not done.
- Nothing of the surface-karst hooks is called by the landscape, the ground level or the voxel forms.

## 3. In progress

A background run of `terra groundwater --at limestone|hot|landing --span 20000 --out .out/groundwater --time 1` with the current build; outputs go to `.out/groundwater/{limestone,hot,landing}.txt` and `groundwater-<tag>-{table,springs,conduits}.png`. Next action after it: read them, then run the real-tile test.

An independent review was done once (findings folded in: section 5 "review"). A second review after the rewrite has not been run.

Shared-file state: `ground/fn/GroundwaterStandIn.cpp` is deleted; `World::groundwater` exists; `groundwaterCommand` is registered in `tool/main.cpp` and `tool/Arguments.hpp`; `testGroundwaterBlock`, `testGroundwaterEdge`, `testGroundwaterTile` are registered in `test/Main.cpp`.

## 4. Next steps

1. Read the three runs. Find why so much water is followed no farther in high relief (lag by class in the timing line), fix or raise `WaterReach` to 96 with `LevelSweeps` 36 and `RouteSweeps` 54.
2. Run `test/GroundwaterTile.cpp` (through the suite, or a driver linked against the library if the suite stops earlier).
3. Voxel check: `terra slice --x --z` at the hottest spring and at a karst spring of the limestone run; say what is seen.
4. Yes/no per karst type on seed 42 (D1 pavement and dolines, D2 towers and cones, D3 poljes/gorges/shafts/cave mouths) for the lead. Expected: D1 no (needs the sink and solution hooks in the landscape and the pavement hook in ground/relief), D2 no, D3 partly (shafts and cave mouths exist as voids; poljes no).
5. Second independent review; rows for `docs/simulation.md`; final report.

## 5. Decisions and why

- **No tile reads another tile's ground water.** A build covers its tile plus `WaterReach = 64` cells of the neighbours' finished landscape. Every step is a synchronous sweep; the rings a build spoils from the field edge are counted in `constant/Hydraulics.hpp` (63 of 64). Two tiles compute identical values on shared cells and the water crossing an edge is one number. Rejected: parity passes adopting neighbours' results (cascade of builds, seams not conservative); closed tiles (a divide every 16 km); converged solves on a halo (tile-dependent values).
- **Positions come from global cell indices**, never from a tile origin.
- **Two storeys per column.** Upper: ground above the first bed that passes less than half the recharge; its water runs along that bed (identity `WaterLayer::unit` + `offset`) and leaves where the bed lies bare in the neighbour (`Way::Outcrop`, contact springs) or continues under the neighbour (`Way::Under`). Lower: everything below, to the base of circulation. N storeys were rejected as too heavy.
- **Level, then routing.** `level` is a min-plus envelope from outlets; it only gives directions and shares. Water is moved by `RouteSweeps` gather sweeps, so the budget closes by construction. What is still under way afterwards is `lag` (budget `underflow`).
- **Vents gather.** A spring yields at least `GatherShare` (3) times its own cell's recharge; otherwise it is seepage. Emerging water in a cell that surface water already enters from a cell with a spring or stream is baseflow.
- **`Spring::silica`/`carbonate` are what comes out of the water**, totals are in `SpringRecord`. Carbonate only counts for water charged beyond 1.5 times what the air's gas holds.
- **Heat**: temperature from the rock the water last stood in (`flowDepth` of the vent's node), silica from the hottest rock on its way (`Borne::deep`), cooled by `exp(-CoolingFlow·depth/discharge)`, capped at boiling. Only `geothermAt`.
- **Karst**: nodes flagged karst are sinks of the routing. `drainKarst` then finds for every karst node of the tile's own cells the way that lets its water stand lowest (label-setting search on head, five rounds, a used step uses less head). Outlets: own ground plus an entry head, open water, the lip of a tight bed, or underflow where the soluble rock goes on lower beyond the tile's edge. Conduit nodes stay one cell inside the tile, so no tube reaches an edge.
- **Conduit geometry**: node on the joint plane (zone, else corridor) its way out follows; height just under its head inside the soluble rock, else on the rock's base. Radius `ConduitScale·Q^0.4`. Abandoned levels: same plan lifted, dated by the outlet cell's incision rate.
- **Sinking streams from rock alone** (`swallowRate`). `column.stream` is the landscape's discharge less the share that soaked in; `sinkStreams` follows the losses 24 cells down each course.
- **What the runs showed (earlier build)**: landing (wet sandstone/mudstone lowland): recharge 64 % of runoff, baseflow 74 %, underflow 5.5 %, table 50 m deep on interfluves, contact seeps on every cell of interbedded slopes (led to `GatherShare`). Hot ground (248 K/km granite mountains): table pinned near the 220 m base (led to the two-half deep profile), 22 boiling springs on 2 100 km² mostly away from faults (led to heat from the last rock). "Limestone" search found marble bands of an orogen (led to the flat-lying search), sunk water exceeded recharge (led to `column.stream` and the stream condition on shafts).
- **Review (first pass), fixed**: reach of `springsIn`, own-ground karst springs along tile edges, carbonate of cold springs, rock of upper-storey conduits, `massOf` for the conduit planes, deep layer, shafts, name clashes (`StreamFlow`, `Carried`, `Reach`, `Layer`), volcanic pile, `gaining` threshold, conduit ids, settle order. Left: `Aquifers` does not recover from an exception thrown in a build (as `Atlas`); `sinkStreams` forgets losses more than 24 cells upstream.
- Trap: the scratchpad directory is shared by all agents of the session. Use a subdirectory; this stream's scratch build is in `scratchpad/groundwater/`.
- Trap: other streams edit `@simulation` concurrently and share `.out/simulation/build`; the library may fail to build for reasons outside this domain, and `Tile` grows fields (`volcanic`). Re-read `landscape/type/Terrain.hpp` before trusting this document about it.

## 6. Contracts with other streams

- `groundwater/type/Spring.hpp`, `Cave.hpp`: signatures unchanged; `silica`/`carbonate` documented as what comes out.
- `simulation/type/World.hpp` (lead's): include and member `Aquifers groundwater;` after `atlas`.
- `ground/fn/GroundwaterStandIn.cpp` (ground owner's): deleted.
- `tool/main.cpp`, `tool/Arguments.hpp`: `groundwaterCommand`. `test/Main.cpp`: three test functions.
- **Landscape runoff hook** (not wired): `landscape/fn/Uplift.cpp`, `readClimate`, after `domain.runoff[i] = runoffOf(...)`: `domain.runoff[i] *= 1 - infiltrationShare({rain, warmthNow, slope, {coarse, sand, fines}, ice, domain.unit[i].traits.dominant})` (`groundwater/fn/Recharge.hpp`). Return path: `GroundwaterTile::emergence` (m³/yr per Local cell) of the parent tile into `domain.inflow` of Site tiles. Until then the infiltrated share is counted twice; the tool prints it.
- **Karst sink hook** (not wired): `swallowRate(SwallowBed)`, `swallowed(rate, discharge, cell)`, `sinkSolution(solubility, sunk, cell)` in `groundwater/fn/Swallow.hpp`. In `landscape/fn/Flow.cpp`, `routeWater`, before a cell's water is shared out: `water -= swallowed(rate[cell], water, cell)`, recording the loss per cell in the tile; in `landscape/fn/Flood.cpp`, `floodLevels`, cells with `rate > 0` are pushed as outlets at their own surface so closed depressions on soluble rock drain; in `Hillslope.cpp` the lowering gains `sinkSolution`. Once the tile records the loss, `readColumns` reads it and `fn/Sinks.cpp` goes.
- **Pavement hook** (not wired): `pavementOf(rock, cover, runoff)`, `grikeDepth(seed, pavement, mass, material, depth)`, `grikeShare` in `groundwater/fn/Pavement.hpp`, for the ground level's cells and the voxel column.
- **Inspector**: `explainSpring(World&, Vec2, reach)`, `explainCave(World&, x, y, z)`, `SpringRecord`, `ConduitRecord` in `groundwater/type/Groundwater.hpp`.
- **Engine**: `groundwaterAt(World&, TileKey)` blocks; `World::groundwater.find(key)` never does.

## 7. How to build, run and verify

- `mise -C @simulation run build`, `mise -C @simulation run test`.
- `.out/simulation/build/macosx/arm64/release/terra groundwater [--x --z | --at landing|limestone|hot] [--span m] [--out dir] [--tag name] [--time 1] [--explain 1] [--seed n] [--age yr]`.
- Tests alone, when the suite stops earlier or the library does not build: `scratchpad/groundwater/build2.sh` compiles the domain and links `gwblock` (block and edge tests) and `gwtest tile` against the last good archive.
- Cost (16 cores, shared): planet to 4.6 Gyr 19 s; a Local landscape tile 6.5 s on one thread; ground water of a Local tile 0.7–1.1 s on one thread, 0.3–0.4 s with a busy pool; `caveSpans` 0.03–0.05 µs a column.

## 8. Open questions

- One spring per 64 m of contact, or should the site refinement split a contact vent among the joint corridors crossing it?
- Cave mouths: the passage ends at the Local level's ground plus 12 m; whether it reaches daylight on the 4 m and 0.25 m ground is unseen.
- `WaterReach` 64 or 96: decided by the underflow share in high relief.
- Should `massOf` live in `rockmass` so that ground water does not include a `ground` header?
