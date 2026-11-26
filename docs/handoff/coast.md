# Hand-over: coast

Describes the present state of the `coast` workstream. Rewrite it when the state changes; it is not a diary.

## 1. Goal

Own `@simulation/src/coast/**` (+ `test/Coast*.cpp`, `tool/Coast*.cpp`): shores of seas and lakes that arise from simulated waves working on what the land offers, at the Region, Local and Site levels. Wanted outcomes, checked against `docs/reference-landscapes.md` (G3, G4, H1, H2, F2): cliffs with a platform at their foot; coves, headlands, stacks, arches and sea caves along joints; beaches sorted by grain size; spits, barriers, lagoons, tombolos; a lake with a sandy beach findable on seed 42. Nothing placed, nothing painted by noise.

## 2. State now

Every hook is in and compiles; the coast runs in every Region, Local and Site replay. Files: `coast/{constant/Waves.hpp, type/{Shore,SeaVoid}.hpp, fn/{Coast.hpp, Waves, Shore, Cliff, Drift, Seas, Grid, Survey, Scenario, SeaVoid}.cpp}`, `test/Coast.cpp`, `test/CoastVoid.cpp`, `tool/{Coast.hpp, CoastCommand, CoastMap, CoastFind, CoastShow}.cpp`.

- `testCoast` (registered last in `test/Main.cpp`; the suite stops earlier at `testLandscapePlane`, so run it with the driver in section 7) passes on the tree of 26 Nov ~09:50: sea voids, rock coasts (cliffs, platforms p50 162 m on chalk, chalk faster than limestone, joints first, shelter), oblique-wave spit vs square waves, lake beach sorting, thread independence of the full step loop.
- Second-run fixes (26 Nov): standing water judged against `Domain::surface` (the ground of the flood), so channels cut after the flood are not lakes; a water's level is not carried down its outlet; wave, surf and load state cleared each step (stale trains on dry ground); a face goes back by several cells in a step when its rate allows (time carried to the ground it lays bare, up to 8 cells, waves raised again in between); mud of fallen columns is put in the water after the waves are raised again and drifts across a level bench instead of burying it; the drift of a non-evolving cell is its capacity times its loose bed over 1 m (a film gives in proportion); undercut of a bluff is bounded by the train's spare capacity; surf rock waiting at the end of a replay is laid even where the water has gone; `book` covers the whole domain; sea caves only where the waves are cutting the face (`notch` ≥ 0.3 m), on master joints (spacing 8–60 m), as long as the cut along the crushed joint runs ahead of the face (no hash share any more).
- Real world, seed 42: `terra coast --find 1` (section 7) ranks coasts and lakes on the Region and builds Local and Site tiles at the best of them. Results: section 3 / the final report of this run.

## 3. In progress

Nothing half-done in code: every file compiles in the shared build (26 Nov 09:51) and `testCoast` passes.

Seed 42, `terra coast --find 1 --tries 4 --pick lake4,lake3,cliff2,sea1 --out .out/coast/find2` (Region 3 × 3 tiles 174–216 s; each Local 2 × 2 survey 11–17 tiles in 31–78 s; each Site 2 × 2 survey 17 tiles in 55–74 s; each view 6–8 site tiles in 28–140 s; load 35–55):

- **lake3** (X 5241344 Z −5961216, Region 49 km², 69 m deep). Local: lake shore 314 km, 31 % sand beach, 62 % low rock, beach p50 128 m. Site at its sand: 84 % sand beach, slope 8.8°, waves p50 0.05 m. The pictures (`view-lake3-sand-{water,shore}.png`) show a flat sandy pan with shallow pools and bars, not an open lake behind a beach: the Region lake survives at Local only as fragments.
- **lake4** (X 5240320 Z −6189568, Region 167 km², 160 m deep). Local: 284 km of lake shore, 23 % sand, but no single large water near the place. Site: 87 % sand around many small ponds; views show a dune-and-pond field.
- **sea1** (X 5044736 Z −5871104). Local: sea shore 692 km — islands, inlets, bays, 40 % sand beach, 52 % low rock, 12.5 km of cliff (p50 19 m, p95 124 m) with a platform p50 38 m, 12.9 km of shore on spits/barriers, lagoons 2.2 km², waves p50 0.20 m, highest 0.95 m. Site at its sand: 81 % sand, beach p50 12 m, slope 11.2°. The water view shows an island ringed by a narrow sand fringe with an offshore bar; the shore view a smooth sand slope into still water.
- **cliff2** stalled for over 12 min in a single-threaded volcanic `buildCone`/`relax` in the scratch binary (linked against an older library). Re-run with the shared terra of 09:56 into `.out/coast/find3/` (8.5 min in all). The tree had changed by then, so the Region ranking differs and cliff2 is a different place (X 4831744 Z −5887488, 1076 m within 2 km). Local: sea shore 644 km, 50 % mud shore, 33 % sand, cliffs p50 117 m (p95 227 m) with a platform p50 31 m, lagoons 42.6 km², waves p50 0.25 m, highest 1.40 m. The map also shows a straight seam across the middle and rectangular terraced outlines (lower centre) that no shore process makes; not traced. Site at the highest cliff: a lake that stops at a tile edge in a 120–330 m gorge, waves 0 — valley walls and a seam, not a wave-cut coast. Site at its sand: 100 % sand, waves p50 0.40 m, a thin sand bar ringing a lagoon offshore with dunes behind (`view-cliff2-sand-water.png`), and a shoreline stepped at 45° in the map.

## 4. Next steps

1. A lake with a real beach (H1) is not yet findable on seed 42: the deep Region lakes break up at Local into ponds and sand flats. That is first a landscape question (item 3), then a wave one (item 2).
2. Wave height on the real Local/Site shores is small (p50 0.05–0.3 m): shelves and lakes are a few metres deep over kilometres and bed friction takes the waves. Check against Balaton-like shallow lakes (storm waves ~1 m at 3 m depth) and decide whether `BedFriction`/`SmoothBed` are too strong.
3. Lakes: Region lakes shallower than 45 m are levelled when a Local domain is built (`levelHollows`, landscape owner); the finder therefore only takes lakes at least 50 m deep. Raise with the landscape owner.
4. Block alignment test (`test/GroundTile.cpp`) with a point in a sea void: needs a site with a wave-cut face in the test world; not done.
5. Rows for `docs/simulation.md`: sent to the lead in the 26 Nov report (section 5 has the content).
6. Straight lines along tile edges are visible in Local and Site maps (`coast-find-*-L2.png`). Whether the coast contributes (drift handed in at the open rim) is not checked: render one Local tile pair with `workShores` off and compare.
7. `recordShore` writes into the shared `Shore` through a `const Domain&` (review, not fixed).

## 5. Decisions and why

- **One hook after `cutAndFill`.** Rivers deliver first, then waves rework what they brought. The face above a wave-cut foot is not failed by coast code: the next step's `breakSlopes`/`settleDebris` (landscape) do that.
- **Waves as eight trains** (one per neighbour direction), state = H² in m². Carried cell to cell downwind (`sweepDownwind`), grown by fetch (SMB: `FetchGrowth·U²` per metre, capped at the fully developed sea), cut by depth (`H ≤ 0.5·depth at high water`) and by bed friction (`dH/dx = −0.0106·H²/h²`). Land lets nothing through, so shelter and exposure by the angle of the coast to the wind need no extra rule; a 20 % blend from each side stands for refraction and directional spread. Rejected: ray tracing per shore cell (cost, and no natural boundary from a parent level).
- **Wind**: every quarter gets `StormGain·(|W|·cos² + StormCalm)`; trains are not weighted by frequency.
- **Still water** (`spreadWater`): breadth-first from cells the flood left wet, through every cell covered at high water (`level + swing`), plus the land cells next to them. `swing` is `SpringTide` (1 m) for the sea and 0.25 m for a lake. `Shore::stood` is captured once per step because `Domain::level` of a cell the coast lowers is stale (it would read as a lake at the old cliff top).
- **Rock**: threshold `Hc = 0.063·√(MPa·sound·(1 − 0.85·weakness)/quality)`, rate `0.02·(H − Hc)/Hc²` m/yr per train, averaged over the eight. `Shore::notch` is the cut-back of a column's foot; at cell width the column comes down to the bench (`still − ½·swing`, Region 2 m deeper). Self-limiting: a bench makes the waves break (`Breaker`) and lose height (`BedFriction`), so retreat stops at a width set by rock and waves, independent of the number of steps. At coarse levels the bench narrower than a cell lives in `notch` (sub-cell platform width). Time per step is `min(step, StandTime = 6 kyr)`, cut in `CutSlices = 8` slices; a face cut through before its share of time is spent comes down and hands the rest of the time to the ground it laid bare (waves raised again between slices), so a weak face goes back by several cells in a step and a strong one by none.
- **Mud** of a fallen column is suspended after the waves are raised again: the cloud walks down the bed (across a level bench, away from where it rose), sinks a share only in calm water, settles whole in a calm hollow and otherwise leaves the domain (`CoastLedger.left`). Rejected: laying it at the first local minimum, which buried the new bench.
- **Sea caves** only where the face is being cut (`notch ≥ NotchLeast`), on master joints (spacing 8–60 m), as long as `min(CaveLongest, CaveReach·ln(struck/withstand), faster·notch)`, where `faster` is how much faster the crushed joint goes back than the face. Rejected: a hash share of joints (a stamp).
- **Standing water** is judged against `Domain::surface` (the ground of the flood), and `spreadWater` does not cross into dry ground lower than the water's low stand (beyond its sill): a channel cut after the flood is not a lake, and a lake's level does not run down its outlet.
- **Debris** of a fallen column goes to `Shore::pending` (surf debris, not in the height field), blocks ground down by the rock's `abrasion` over `SurfTravel`. Rejected: putting it on the cell as cover — a 60 m column would leave a 50 m rubble pile, while in nature the debris is removed far faster than a face crosses a cell.
- **Sand**: per train, a load is carried downwind through cells within reach (closure depth 2.5·H); capacity `Carry·E^1.25·reach`; excess is laid (not above high water), deficit is picked up (`Stir` per sweep); a class may not climb a slope steeper than it holds (`SwashSlope / √(1 + depth/0.5)`: gravel 0.27, sand 0.08, fines 0.004), the blocked part is turned to the two cells 45° to either side — that is longshore drift; at the shore the swash lays sand then gravel up to the run-up height. Undertow moves a class back down where it lies steeper than it holds; fines only ever move that way. Rejected: pairwise slope relaxation (anti-diffusive for slopes below equilibrium, one cell per round), and a full CEM shoreline tracer (too large).
- **`Domain::trend` is moved by what waves cut and lay** (in `book`, `Shore.cpp`). Without it a finer level's found uplift (`landscape/fn/Uplift.cpp`) would heal a bench or a spit back onto the parent's surface. Approved by the lead.
- **Traps**: every border cell of a domain must be `Rim` (`raiseWaves` reads three neighbours of every other cell). `Domain` copies share one `Shore` (`shared_ptr`). `workShores` returns before allocating anything when no water stands.

## 6. Contracts with other streams

Volumetric query (`coast/type/SeaVoid.hpp`), approved by the lead:

```cpp
struct SeaVoid { double ax, az, bx, bz; float y, wide, high, water; SeaVoidKind kind; };
struct SeaVoids { std::vector<SeaVoid> voids; };
inline constexpr double SeaVoidReach = 36.0;   // plus one site cell; static_assert against BlockSiteReach in Block.cpp
using SiteTiles = std::function<const Tile*(int32_t x, int32_t z)>;
SeaVoids seaVoidsOver(const SiteTiles& tiles, const Setting& setting, double minX, double minZ, double maxX, double maxZ);
int  seaVoidSpans(const SeaVoids&, Vec2 position, CaveSpan* spans, int capacity);
bool inSeaVoid(const SeaVoids&, double x, double y, double z);
void cutSeaVoids(const SeaVoids&, double x, double z, double bottom, double step, int count, Material* out, size_t stride);
```

Lines in shared files, all **in**:

- `landscape/type/Terrain.hpp`: `struct Shore;`, `struct ShoreRecord;`; `std::shared_ptr<const ShoreRecord> shore;` in `Tile`; `std::shared_ptr<Shore> shore;` in `Domain`.
- `landscape/fn/Process.hpp`: `workShores` declaration. `landscape/fn/Replay.cpp`: `workShores(domain, scratch);` after `cutAndFill`. `landscape/fn/Build.cpp`: `readSeas(domain, setting, lookup);` after `openRim`. `landscape/fn/Extract.cpp`: `tile.shore = recordShore(domain);`.
- `simulation/fn/Block.cpp`: `seaVoidsOver` once per block, `cutSeaVoids` after `formColumn` per column, and (26 Nov) `static_assert(BlockSiteReach >= SeaVoidReach + specOf(Level::Site).cell, ...)`.
- `test/Main.cpp` (26 Nov): `void testCoast();` and `testCoast();` after `testVolcanicLaws`. `tool/main.cpp`, `tool/Arguments.hpp`: `coast` command.

Other owners: landscape and its stability rules (relief stream), `rockmass`/`strata`/`planet` (lead), `measure`, `groundwater`, wind and dunes (aeolian stream; dry beach sand above high water is ordinary Sand cover for it).

## 7. How to build, run and verify

```sh
mise -C @simulation run build     # library, terra, tests (build directory is shared with other streams)
.out/simulation/build/macosx/arm64/release/terra coast --scene straight|bay|lake ...   # hand-built shores, map + view
.out/simulation/build/macosx/arm64/release/terra coast --find 1 [--tries 4] [--pick lake1,cliff2,sea1] [--confirm 0] --out .out/coast/find
.out/simulation/build/macosx/arm64/release/terra coast --x m --z m --level 2|3 --tiles 2 --out dir
```

`--find` ranks lowland sea coasts (`sea`), steep ones (`cliff`) and lakes at least 50 m deep (`lake`) on the Region within 300 km of the landing, then for each picked one surveys 2 × 2 Local tiles, 2 × 2 Site tiles at its best sand beach (and highest cliff) and renders a picture from the water and one from the shore (`view-<tag>-<kind>-{water,shore}.png`). A fresh process rebuilds the planet and the Region (3–8 min under load).

Fast loop without touching the shared build: compile `src/coast/fn/*.cpp` (and `tool/*.cpp` for terra) into a scratch directory and link them before a copy of `libderivean-simulation.a`; the coast tests need a `main` calling `testCoast()` linked with `test/Coast.cpp test/CoastVoid.cpp test/LandscapeFixture.cpp`.

## 8. Open questions

- Wave spread (`WaveSpread`, 20 % to each side per cell) is a per-cell blend, so the lateral spread per metre depends on the cell size (independent review, 26 Nov). A directional spread in angle would need the trains' energy to be moved by distance, not by cells.
- The rock threshold `Hc ∝ √(strength)` is a calibration in Sunamura's form, not his linear one; it compresses the chalk-to-granite contrast to about 5× (review). Kept because the rates it gives (chalk decimetres a year, granite nothing) match the catalogue; to be revisited with measured retreat rates.
- `WaveQuantum` caps recorded heights at 12.75 m; a fully developed sea under a 10 m/s mean wind is higher. Only the Region's open rims can reach it.
- `Carry` and `Stir` are per sweep, not per year: the drift of a level follows its step count. Spits grow slowly.
- Drift handed in at the open rim and by Settled cells stands for the shore beyond as it is (capacity × loose bed over 1 m); it is booked in `CoastLedger.entered`, not in `TileLedger.sedimentIn` (which is the rivers'). Across tiles this is a boundary condition, not conserved mass.
- Glacial low stands of the sea are not in the finer levels' sea level; drowned valleys exist only where the Region's dated sea level rose.
- Survey: `platformWidth` takes the shortest platform run over a shore cell's wet sides, which along a shore-parallel strip can read kilometres (the "widest" figure); p50/p95 are the trustworthy ones.
