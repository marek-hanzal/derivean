# Hand-over: ground water, springs, caves, karst

Owner of `@simulation/src/groundwater/**`, `@simulation/test/Groundwater*.cpp`, `@simulation/tool/Groundwater*.cpp`. Nothing is committed.

## 1. Goal

The owner wants a world that is not empty and not a height map. Springs, cascades, caves, sinkholes and hot ground must arise in the simulation, never be placed. Water must come from somewhere (rain that infiltrated), and whatever a player finds must be explainable ("why is there a spring here?"). The world has no life. Karst is this stream's in full: caves and the surface forms (sinking streams, dolines, pavement, poljes, towers). The surface forms can only be delivered as hooks, because `landscape/**`, `ground/**`, `relief/**` and `inspect/**` belong to others.

## 2. State now

Builds: `mise -C @simulation run build` succeeded with everything below in it (library, `terra`, `simulation-tests`).

Works and how it was checked:

- Whole pipeline for a Local tile: columns from cover, volcanic pile and beds; recharge; sinking streams; levels; routing in two storeys; springs with heat and loads; conduit network; cave field; `caveSpans`/`inCave`; `springsIn` with site refinement (`fn/Refine.cpp`); explanation records; per-world cache `World::groundwater`.
- `WaterReach` is 96 (`LevelSweeps` 36, `RouteSweeps` 54): water is followed 3.5 km.
- `test/GroundwaterBlock.cpp`, `test/GroundwaterEdge.cpp` pass: contact spring line (256 springs on 256 cells of outcrop, 99.9 % of the recharge), limestone block (sunk stream of 95 l/s rises 10.9 km away, 124 km of passages over 0.5 m, 97 % along joint zones, spans checked against the tubes, every karst vent inside the cell its water left), fault spring (98 °C, 18 % steam, silica with heat; 34 °C and nothing without), two hand tiles agree to the bit along their shared edge.
- `test/GroundwaterTile.cpp` passes with reach 96 on the young real world (1 thread = pool bitwise, real shared edge bitwise, cache from three threads, `checkVents`, `explainSpring`): underflow 32–36 % of the recharge (it was 59.6 % with reach 64 and failed the 50 % bound). Ran through `scratchpad/groundwater/s2/gwtests4 tile` (driver linked against the built archive), not through the suite, which stops earlier.
- Real springs and caves reach the voxel answer (`scratchpad/groundwater/s2/voxel.cpp`: refines a vent, checks `springsIn` lists it, samples a 48 m block through `sampleBlock`, follows passages near the vent against the ground level's surface):
  - Hottest spring of the hot site (karst rising in chalk, 99.8 °C, 10 % steam, 4.8 l/s, X 3542562 Z 1172640): the 48 m block's surface has sinter 1.0 %, travertine 0.2 %, altered clay 5 %, a pool 1.3 m deep at the vent. Its mouth shaft ended 6.9 m under the finer ground (site ground 7.3 m above the local level's); with `MouthRise` it now reaches daylight.
  - Karst spring with most passages there (115 l/s, 49 °C, 15 km of passage): 271 of 9 216 columns have air under rock, 1 091 void voxels, travertine 2.4 % of the surface; the mouth opens.
  - Karst country (X 3554336 Z 1730336, chalk lowland, 199 l/s, 31 km of passage): 143 roofed columns, 1 772 void voxels; the mouth rises from 20 m to 12 m above the ground and opens once.
- `terra groundwater --at limestone` now finds carbonate the landscape actually has (section 5). Seed 42, build of 09:30: chalk lowland at X 3530752 Z 1744896, 2 m above the sea, 500 km² of land in 2 416 km²: 94 % of the land drains by conduits, 21 307 cells where streams sink, 30 092 conduits, 1 900 km of passage (4 km over 2 m radius), 232 km of abandoned levels, 251 karst springs up to 140 l/s. With the landscape of 10:00 (other streams' edits) the same coast is drowned: 44 km² of land, 1 200 conduits, karst springs up to 2.7 l/s.
- Final runs on the final build, `.out/groundwater/final/{hot,limestone,landing}.txt` and maps: hot site (266 K/km) underflow 17.8 % of what enters, 72 fault and 50 karst springs, hottest 74 °C, largest karst spring 113 l/s with 11 km of passage, 199 km of passage, 1.2 km of shafts; landing underflow 7.0 %, 61 fault springs; one Local tile's ground water 1.3–3.8 s on one thread and 0.3–0.45 s with the pool at load about 35 (reach 64 was 0.6–1.2 s and 0.1–0.3 s).
- Voxel check repeated on the final build (`s2/voxel-hot4.txt`): hottest spring of the hot site now a 74 °C table spring in basalt with 3.8 % travertine on the block's surface; the 113 l/s karst rising has 202 roofed columns, 602 void voxels, both mouth passages open to the surface.

Not verified:

- The place results move with every landscape change (the hot site's boiling spring of the morning is gone at 10:00; the chalk coast drowned). Numbers here are of builds of 26 Nov morning.
- Timing of the real-tile test inside the suite; cost of reach 96 on an idle machine (at load 40: 0.35 → 0.50 s and 0.42 → 0.78 s a tile with the pool).
- Nothing of the surface-karst hooks is called by the landscape, the ground level or the voxel forms: no dolines, pavement, poljes or towers anywhere.
- That the finer ground at a vent stays within `MouthRise` (12 m) of the local level's: seen +2, +7.3 and −13 m at four vents.

## 3. In progress

Nothing. No edit is half-done and no run of this stream is going.

Second independent review done after the takeover (findings and what was done: section 5).

Shared-file state: `ground/fn/GroundwaterStandIn.cpp` is deleted; `World::groundwater` exists; `groundwaterCommand` is registered in `tool/main.cpp` and `tool/Arguments.hpp`; `testGroundwaterBlock`, `testGroundwaterEdge`, `testGroundwaterTile` are registered in `test/Main.cpp`. No shared file was touched in the second run.

## 4. Next steps

1. Regional flow: under faulted plateaus and confined beds 60–80 % of the recharge travels farther than any affordable reach (section 5). A ground-water pass at the Region level (1 km cells, a region tile is 262 km) would take the Local underflow and bring it out at major valleys; until then it comes out nowhere.
2. Wire the hooks (landscape owner): runoff less recharge, karst sinks and closed depressions, pavement. Without them D1 and D2 stay impossible.
3. Re-run the three places after the relief rework lands; the landscape under them changed during this run already.
4. Open items from the second review (section 5).

## 5. Decisions and why

- **No tile reads another tile's ground water.** A build covers its tile plus `WaterReach = 96` cells of the neighbours' finished landscape. Every step is a synchronous sweep; the rings a build spoils from the field edge are counted in `constant/Hydraulics.hpp` (95 of 96, the reviewer counted 93: about 2 slack). Two tiles compute identical values on shared cells and the water crossing an edge is one number. Rejected: parity passes adopting neighbours' results (cascade of builds, seams not conservative); closed tiles (a divide every 16 km); converged solves on a halo (tile-dependent values).
- **Positions come from global cell indices**, never from a tile origin. A spring's id hashes the global cell its water left the ground in (not the cell its position falls in: a lip or joint plane can sit on the cell's edge), and every vent lies inside that cell.
- **Reach 96 / 54 sweeps.** Lag (water still under way) by reach 64 / 96 / 128 cells: hot granite tile 17.9 / 5.9 / 3.5 %, faulted sandstone plateau 83.6 / 76.6 / 58.7 %, test tile 59.6 / 32 % / not run. 95 % of the lagging water on the plateau sat within 8 cells of a stream and never entered it: the rest level beside the stream is set by an outlet farther down the valley and lies below the stream's stage; in fault rock (T 5e5–1e6 m²/yr) or under a tight bed (confined head 70 m under the ground) the water is a regional flow below the streams. Rejected: shares that depend on the flow a node passes (a node's stand rises above rest until it passes its flow, reaching streams and lips above its rest level): plateau 83.6 → 76.3 %, hot tile 17.9 → 20.3 %, because the stand needs only metres in such rock; letting it reach any neighbour below the ground sent water back uphill. Reach 128 was rejected on cost (about 4× the routing of 64).
- **Two storeys per column.** Upper: ground above the first bed that passes less than half the recharge; its water runs along that bed (identity `WaterLayer::unit` + `offset`) and leaves where the bed lies bare in the neighbour (`Way::Outcrop`, contact springs) or continues under the neighbour (`Way::Under`). Lower: everything below, to the base of circulation. N storeys were rejected as too heavy.
- **Level, then routing.** `level` is a min-plus envelope from outlets; it only gives directions and shares. Water is moved by `RouteSweeps` gather sweeps, so the budget closes by construction. What is still under way afterwards is `lag` (budget `underflow`).
- **Vents gather.** A spring yields at least `GatherShare` (3) times its own cell's recharge; otherwise it is seepage. Emerging water in a cell that surface water already enters from a cell with a spring or stream is baseflow.
- **`Spring::silica`/`carbonate` are what comes out of the water**, totals are in `SpringRecord`. Carbonate only counts for water charged beyond 1.5 times what the air's gas holds.
- **Heat**: temperature from the rock the water last stood in (`flowDepth` of the vent's node), silica from the hottest rock on its way (`Borne::deep`), cooled by `exp(-CoolingFlow·depth/discharge)`, capped at boiling. Only `geothermAt`.
- **Karst**: nodes flagged karst are sinks of the routing. `drainKarst` then finds for every karst node of the tile's own cells the way that lets its water stand lowest (label-setting search on head, five rounds, a used step uses less head). Outlets: own ground plus an entry head, open water, the lip of a tight bed, or underflow where the soluble rock goes on lower beyond the tile's edge. Conduit nodes stay one cell inside the tile, so no tube reaches an edge.
- **Conduit geometry**: node on the joint plane (zone, else corridor) its way out follows; height just under its head inside the soluble rock, else on the rock's base. Radius `ConduitScale·Q^0.4`, at most half the soluble run's thickness. Abandoned levels: same plan lifted, dated by the outlet cell's incision rate. The mouth runs from the outlet node to the vent (12 m beyond it where the node lies aside) and rises to the local ground + `MouthRise` (12 m), so it opens wherever the finer ground lies within that; above the ground the tube is air. Where node and vent coincide the mouth is a vertical pit of the passage's radius. `ConduitRecord::opens` marks mouths (2) and shafts (1).
- **The limestone search reads the region.** The Region level cuts far into the layering the planet records (datum minus rock: 1.6 km under the first "limestone" place, 300 m on the chalk lowland, 4.7–6.3 km at three mountain candidates), so the planet's carbonate at the top of a column is often gone in the landscape. `tool/GroundwaterSeek.cpp` ranks planet places by carbonate in the top 2 500 m, builds the regions of the best 24 and reads 25 probes 2 km apart: the 150 m under the region's cut, wet probes counting as none. Reported to the lead: the region seeds its datum at the planet's past elevation but reads the present cover, which looks like erosion counted twice (planet/relief owners).
- **Sinking streams from rock alone** (`swallowRate`). `column.stream` is the landscape's discharge less the share that soaked in; `sinkStreams` follows the losses 24 cells down each course.
- **What the runs showed (earlier build)**: landing (wet sandstone/mudstone lowland): recharge 64 % of runoff, baseflow 74 %, underflow 5.5 %, table 50 m deep on interfluves, contact seeps on every cell of interbedded slopes (led to `GatherShare`). Hot ground (248 K/km granite mountains): table pinned near the 220 m base (led to the two-half deep profile), 22 boiling springs on 2 100 km² mostly away from faults (led to heat from the last rock). "Limestone" search found marble bands of an orogen (led to the flat-lying search), sunk water exceeded recharge (led to `column.stream` and the stream condition on shafts).
- **Review (first pass), fixed**: reach of `springsIn`, own-ground karst springs along tile edges, carbonate of cold springs, rock of upper-storey conduits, `massOf` for the conduit planes, deep layer, shafts, name clashes (`StreamFlow`, `Carried`, `Reach`, `Layer`), volcanic pile, `gaining` threshold, conduit ids, settle order. Left: `Aquifers` does not recover from an exception thrown in a build (as `Atlas`); `sinkStreams` forgets losses more than 24 cells upstream.
- **Review (second pass, after the takeover)**, fixed: karst vents on a joint plane up to 45 m from the cell centre (outside their cell, possibly in the next tile, breaking `springsIn` halves and ids) are clamped into their cell and ids hash the cell; conduit radius bounded by the soluble thickness; explicit `opens` flag instead of the sign of `cover`; stale `Cave.hpp` comment; seek printing, "none found" return, region requests ahead. Left: `checkVents` tests one spring and splits only mid-tile; the real-edge comparison leaves out `borne`; no test that conduits and springs agree near a tile edge; `GroundwaterEdge` sets columns by hand (slope and dip ring not exercised); `caveSpans` drops spans beyond its capacity that `inCave` reports; `Aquifers` and exceptions (as before).
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
- Tests alone, when the suite stops earlier: `scratchpad/groundwater/s2/testmain.cpp` (`gwtests block|edge|tile`) linked with the three test files, `GroundwaterFixture.cpp`, `LandscapeFixture.cpp` and the built archive. When the library does not build: `scratchpad/groundwater/s2/variant.sh name reach level route` compiles the domain with other sweep counts and links probes ahead of an archive copy.
- Probes in `scratchpad/groundwater/s2/`: `reach.cpp` (lag per class, distance of lagging water to streams, path traces), `lime.cpp` (what the columns of a tile read: cut below the datum, rocks, soluble runs), `voxel.cpp` (section 2).
- Cost (16 cores, shared): planet to 4.6 Gyr 19 s; a Local landscape tile 6.5 s on one thread; ground water of a Local tile 0.7–1.1 s on one thread, 0.3–0.4 s with a busy pool; `caveSpans` 0.03–0.05 µs a column.

## 8. Open questions

- One spring per 64 m of contact, or should the site refinement split a contact vent among the joint corridors crossing it?
- Cave mouths: 12 m of rise covers the four vents seen; a site ground more than 12 m above the local level's buries the mouth.
- A rising whose node is its own vent opens as a pit as wide as the passage (up to about 8 m): a Vaucluse-type spring, or too many pits?
- Should `massOf` live in `rockmass` so that ground water does not include a `ground` header?
