# Hand-over: ground water, springs, caves, karst

Owner of `@simulation/src/groundwater/**`, `@simulation/test/Groundwater*.cpp`, `@simulation/tool/Groundwater*.cpp`. Nothing is committed.

## 1. Goal

The owner wants a world that is not empty and not a height map. Springs, cascades, caves, sinkholes and hot ground must arise in the simulation, never be placed. Water must come from somewhere (rain that infiltrated), and whatever a player finds must be explainable ("why is there a spring here?"). The world has no life. Karst is this stream's in full: caves and the surface forms (sinking streams, dolines, pavement, poljes, towers), the latter through hooks other owners wire, because `landscape/**`, `ground/**`, `relief/**` and `inspect/**` belong to others.

## 2. State now

Works (library and `simulation-tests` target build and link; `Query.cpp`, `Form.cpp`, `Finish.cpp` compile against the real symbols):

- The whole pipeline for one Local tile: columns from beds, recharge, sinking streams, water levels, routing in two storeys, springs with heat and loads, conduit network, cave field, `caveSpans`/`inCave`, `springsIn` with site refinement, explanation records, per-world cache `World::groundwater`.
- Checked by hand on one real tile (seed 42, age 1.2e9, the landing): budget closes to 3e-9; 1.06 s on one thread, 0.53 s with a busy pool (columns 470 ms, karst 240 ms, routing 190 ms).

Not yet done or verified:

- No test file exists yet (`test/Groundwater*.cpp`), nothing registered in `test/Main.cpp`.
- Tool `terra groundwater` is written and registered but has only been run through a scratch build; its maps have not been looked at.
- The karst criterion was just narrowed (it flagged a whole tile); not re-measured.
- Bit-equality of two tiles' overlap, 1 thread vs pool, hand-built aquifer/limestone/heat cases: designed for, not tested.
- The surface-karst hooks exist as functions (`fn/Swallow.hpp`) but nobody calls them from the landscape.
- Independent review not run.

Broken right now, not by this stream: the `terra` target does not compile (`atmosphere/type/Plume.hpp` defines `AshClass { …, Sand, … }` against `SedimentClass::Sand` of `landscape/type/Terrain.hpp`, seen in `tool/SnowCommand.cpp`), and the last library build failed on `hydrology/constant/Water.hpp: redefinition of 'BedRoughness'`. Both are other streams' in-flight edits. Work-around while that lasts: compile `tool/Groundwater*.cpp` plus a three-line `main` against the last good `libderivean-simulation.a` with `clang++ -std=c++20 -O2 -I @simulation/src -I @simulation/tool`.

## 3. In progress

First real runs of the tool at the landing (age 1.2e9), to judge numbers and maps. Files being edited: `tool/GroundwaterCommand.cpp`, `tool/GroundwaterMaps.cpp`, `tool/GroundwaterTiming.cpp`, and constants in `groundwater/constant/Hydraulics.hpp`.

Shared-file state: `ground/fn/GroundwaterStandIn.cpp` is deleted; `World::groundwater` (`Aquifers`) exists in `simulation/type/World.hpp`; `groundwaterCommand` is registered in `tool/main.cpp` and `tool/Arguments.hpp`.

## 4. Next steps

1. Look at the three maps at the landing; fix what is wrong (spring density, karst extent, table depth).
2. Tests in `test/GroundwaterTile.cpp` (real tile: budget closure, two neighbouring tiles agree on their shared ring to the bit, 1 thread vs pool bitwise, order independence) and `test/GroundwaterBlock.cpp` (hand-built 256² fields with `margin = 0`: tilted sandstone on shale gives a spring line at the contact; limestone block with a head difference grows a through-going conduit that `caveSpans` finds; a heat source gives a hot spring with silica and steam and none without; `caveSpans` equals `inCave` for any alignment). Register one line each in `test/Main.cpp`. Budget about 40 s.
3. Three real places found by search, not coordinates: `findLanding`; soluble rock (`terra find --rock Limestone`, `terra scout`); the hottest humid ground (`findHotGround` in `measure/fn/ScoutHeat.cpp`, or scan `geothermAt`). Run `terra groundwater --x --z --span 48000 --out .out/groundwater --tag <name> --time 1`, read the PNGs, compare with catalogue D1–D3, E5 and the spring and cave-mouth rows of table K in `docs/reference-landscapes.md`.
4. Voxel check the lead asked for: a hot spring site and a cave mouth through `terra slice` / `terra ground`, say what is seen.
5. Surface karst: pavement closed form for the ground/relief owner (grike half-width and depth along `jointDistance` order 0 on bare soluble rock), doline feedback (`sinkSolution`), and the exact hook lines (section 6). Report yes/no per karst type D1, D2, D3 on seed 42.
6. Independent reviewer sub-agent (determinism, conservation, thread safety, AGENTS.md); fix findings.
7. Rows for `docs/simulation.md` (evolved / closed form / seeded / absent) and the final report.

## 5. Decisions and why

- **No tile reads another tile's ground water.** A build covers its tile plus `Reach = 64` cells of the neighbours' finished landscape. Every step is a synchronous sweep (a cell reads its eight neighbours as they stood before the sweep), and the sweeps add up to less than `Reach`: `max(LevelSweeps 24, SinkSweeps 24) + RouteSweeps 34 + 5 < 64` (static_assert in `Hydraulics.hpp`). So two tiles compute identical values on shared cells and water crossing an edge is the same number on both sides. Rejected: parity passes with adoption of neighbours' results (cascade of builds, seams not conservative); closed tiles (a groundwater divide every 16 km); converged solves on a halo (tile-dependent values, conduits would not match across edges).
- **Positions are computed from global cell indices**, never from a tile origin, for the same reason.
- **Two storeys per column.** Upper: ground above the first bed that passes less than half the recharge (`PerchShare`); its water runs along that bed (identity `Layer::unit` + `offset`, equal within `SameSheet`) and leaves where the bed crops out (`Way::Outcrop`, contact springs) or continues under the neighbour (`Way::Under`). Lower: everything below, to the base of circulation. A full N-storey network was rejected as too heavy for what it adds.
- **Level, then routing.** `level` is a min-plus envelope: outlet levels (streams, lakes, sea, outcrops) rising by the least gradient that carries a hillside's recharge, never under the floor or over the ground. It only gives directions and shares. Water is then moved by `RouteSweeps` gather sweeps, so the budget closes by construction whatever the level's accuracy. What is still under way after the last sweep is booked per node as `lag` (budget `underflow`).
- **Emergence** is flux a node cannot pass sideways with its water at the ground (`room`). Water under a tight bed does not come up through it.
- **Springs** are heads only: emerging water in a cell that already has surface water coming in from above (by the landscape's `flowTo`) is baseflow. Below `LeastSpring` (0.03 l/s) it is seepage.
- **`Spring::silica` and `carbonate` are what comes out of the water, not what is in it**: silica above what cool water keeps against glassy silica, carbonate above balance with the air. Totals are in `SpringRecord`. Reason: the ground level lays a fixed share of these as sinter and travertine; with totals every cold limestone spring would grow terraces in a world without life.
- **Heat**: vent temperature is the rock's at the depth the water reached (`Carried::deep`; on a fault `min(crush²·FaultDepth, DriveRatio·head)`), cooled on the way up by `exp(-CoolingFlow·depth/discharge)`, capped at boiling for the elevation; the excess flashes to steam. No heat pattern of its own: only `geothermAt`.
- **Karst is closed per tile; the diffuse system is not.** Nodes flagged karst are sinks of the routing (`NodeFate::captured`); `drainKarst` routes exactly that water to outlets inside the tile by five rounds of cheapest-path search in which a used path gets easier with the flow it carried (so flow gathers and large outlets capture small ones). Water with no outlet within `LastResort` is underflow. Conduit nodes are kept one cell inside the tile so no tube reaches an edge; the contract's "listed in both tiles" holds vacuously. Bounded-reach evolution was rejected: capture needs many rounds over kilometres.
- **Conduit geometry**: node positions are projected on the joint plane (zone, else corridor) their way out follows, from `jointDistance` through `fissuresAt`; height is the water table inside the soluble run, or the tight bed for perched systems. Radius `ConduitScale·Q^0.4`. Abandoned levels are the same plan lifted by `max(8, 3r+4)` m, dated by the outlet cell's incision rate; they exist only where rock and roof allow.
- **Sinking streams from rock alone** (`swallowRate`), as the lead required, so the landscape can use the same number. Until the landscape is wired, `sinkStreams` estimates how much of each stream is left along its course.
- **Deep loss** uses `DeepGradient = 3e-5` of the base rock's conductivity; 0.01 swallowed all recharge in sandstone.
- Trap: hand-built fields must have `size - 2·margin == TileCells`; per-cell arrays of `GroundwaterTile` assume it.
- Trap: `massOf`/`rockMassAt` must be asked with the layering node's column and, for planes shared with other readers, `canonicalThickness`.

## 6. Contracts with other streams

- `groundwater/type/Spring.hpp`, `Cave.hpp`: signatures unchanged. Meaning of `silica`/`carbonate` narrowed as above (comment in `Spring.hpp` still to be updated).
- `simulation/type/World.hpp` (lead's): `#include "groundwater/service/Aquifers.hpp"` and member `Aquifers groundwater;` after `atlas`.
- `ground/fn/GroundwaterStandIn.cpp` (ground owner's): deleted.
- `tool/main.cpp`, `tool/Arguments.hpp`: one line each for `groundwaterCommand`.
- **Landscape runoff hook** (not wired): in `landscape/fn/Uplift.cpp`, `readClimate`, after `domain.runoff[i] = runoffOf(...)`: `domain.runoff[i] *= 1 - infiltrationShare({rain, warmthNow, slope, {coarse, sand, fines}, ice, domain.unit[i].traits.dominant})` (`groundwater/fn/Recharge.hpp`). Return path: `GroundwaterTile::emergence` (m³/yr per Local cell) of the parent tile into `domain.inflow` of Site tiles. Until then the infiltrated share is counted twice; the tool prints the numbers.
- **Karst sink hook** (not wired): `swallowRate(SwallowBed)` and `swallowed(rate, discharge, cell)` in `groundwater/fn/Swallow.hpp`. In `landscape/fn/Flow.cpp`, `routeWater`, before a cell's water is shared out: `water -= swallowed(rate[cell], water, cell)`, recording the loss per cell; in `landscape/fn/Flood.cpp`, `floodLevels`, cells with `rate > 0` are pushed as outlets at their own surface so closed depressions on soluble rock drain instead of filling. Solution feedback for dolines: `sinkSolution(solubility, sunk, cell)` added to the lowering in `Hillslope.cpp`. Once the landscape records the loss in the tile, `readColumns` takes it and `fn/Sinks.cpp` goes.
- **Inspector data**: `explainSpring(World&, Vec2, reach)`, `explainCave(World&, x, y, z)`, `SpringRecord`, `ConduitRecord` in `groundwater/type/Groundwater.hpp`.
- **Engine**: `groundwaterAt(World&, TileKey)` blocks; `World::groundwater.find(key)` never does.

## 7. How to build, run and verify

- `mise -C @simulation run build` (library, `terra`, `simulation-tests`), `mise -C @simulation run test`.
- `.out/simulation/build/macosx/arm64/release/terra groundwater [--x --z] [--span m] [--out dir] [--tag name] [--time 1] [--seed n] [--age yr]`.
- Cost on this machine (16 cores, shared with other agents): planet 4.6 Gyr 19 s; a Local landscape tile 6.5 s on one thread; ground water of a Local tile about 1 s on one thread.
- Tests use the young world of `test/LandscapeFixture.hpp` (seed 42, age 1.2e9).

## 8. Open questions

- Is one spring per 64 m of contact enough for "a line of seeps", or should the site refinement split a contact vent among the joint corridors that cross it?
- Cave mouths: the passage ends at the Local level's ground; on steep ground the 4 m and 0.25 m surfaces differ by metres. Needs a look in voxels.
- Whether `Reach` should grow to 96 (longer paths, 36 % more columns) depends on the measured `underflow` share.
