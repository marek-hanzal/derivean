# Hand-over: coast

Describes the present state of the `coast` workstream. Rewrite it when the state changes; it is not a diary.

## 1. Goal

Own `@simulation/src/coast/**` (+ `test/Coast*.cpp`, `tool/Coast*.cpp`): shores of seas and lakes that arise from simulated waves working on what the land offers, at the Region, Local and Site levels. Wanted outcomes, checked against `docs/reference-landscapes.md` (G3, G4, H1, H2, F2): cliffs with a platform at their foot; coves, headlands, stacks, arches and sea caves along joints; beaches sorted by grain size; spits, barriers, lagoons, tombolos; a lake with a sandy beach findable on seed 42. Nothing placed, nothing painted by noise.

## 2. State now

Every hook is in and compiles; the coast runs in every Region, Local and Site replay. Files: `coast/{constant/Waves.hpp, type/{Shore,SeaVoid}.hpp, fn/{Coast.hpp, Waves, Shore, Cliff, Drift, Seas, Grid, Survey, Scenario, SeaVoid}.cpp}`, `test/Coast.cpp`, `test/CoastVoid.cpp`, `tool/{Coast.hpp, CoastCommand, CoastMap, CoastFind, CoastShow}.cpp`.

- `testCoast` (registered last in `test/Main.cpp`; the suite stops earlier at `testLandscapePlane`, so run it with the driver in section 7) passes on the tree of 26 Nov ~11:00: sea voids (with points in and over sea caves read through blocks of drawn size and alignment as `sampleBlock` reads them), wave growth against the Shore Protection Manual (deep 100 km at 15 m/s 2.33 m; 20 km of 3.2 m water at 20 m/s 0.84 m; a swell worn at JONSWAP rates to what the depth holds), rock coasts (cliffs, platforms, chalk faster than limestone, joints first, shelter), oblique-wave spit vs square waves, lake beach sorting, thread independence of the full step loop.
- Second-run fixes (26 Nov): standing water judged against `Domain::surface` (the ground of the flood), so channels cut after the flood are not lakes; a water's level is not carried down its outlet; wave, surf and load state cleared each step (stale trains on dry ground); a face goes back by several cells in a step when its rate allows (time carried to the ground it lays bare, up to 8 cells, waves raised again in between); mud of fallen columns is put in the water after the waves are raised again and drifts across a level bench instead of burying it; the drift of a non-evolving cell is its capacity times its loose bed over 1 m (a film gives in proportion); undercut of a bluff is bounded by the train's spare capacity; surf rock waiting at the end of a replay is laid even where the water has gone; `book` covers the whole domain; sea caves only where the waves are cutting the face (`notch` ≥ 0.3 m), on master joints (spacing 8–60 m), as long as the cut along the crushed joint runs ahead of the face (no hash share any more).
- Third run (26 Nov): waves grow along the SPM 1984 curve of the water's depth (eq. 3-39) instead of deep-water growth plus shallow-water friction, which kept shallow water at about 60 % of measured heights; bed friction acts only on a sea higher than the water holds (swell from deeper water), with the intermediate-depth share of linear theory; `recordShore` raises the final waves on a working copy instead of writing into the shared `Shore`; the survey reports waves offshore (5–15 m of water) beside those at the shore, which are depth-limited by definition.
- Real world, seed 42: `terra coast --find 1` (section 7) ranks coasts and lakes on the Region and builds Local and Site tiles at the best of them. Results: section 3 / the final report of this run.

## 3. In progress

Nothing half-done in code: the shared build passes (26 Nov ~10:58) and `testCoast` passes with the scratch driver.

Measurements of 26 Nov, seed 42, sea1 (Local 3 × 3 tiles from X 5044736 Z −5871104 unless said), outputs in `.out/coast/seams/`, `.out/coast/waves2/`, `.out/coast/waves3/`:

- **Seams with the coast off and on** (scratch builds with `workShores`/`readSeas` empty; driver `seams.cpp` in the scratch directory, numbers are edge pairs against inside pairs). Measured with `seamsOf` (dry cells) the coast changes little: step ratio 0.87 off, 0.80 / 1.13 / 0.62 on (old waves / first new law / final law); bend ratio 1.44 off, 1.70 / 1.63 / 1.53 on. Ground within 3 cells of a shoreline, bend ratio 1.20 off, 1.74 / 1.51 / 0.69 on, from only 270–575 edge pairs, so noisy. The one steady signal: the waves lay 24–37 % more within 2 cells of a tile edge than 10 or more cells in (final law 3.42 m against 2.49 m of |laid| on worked cells; the share of cells worked is the same). The drift boundary at Settled cells (capacity × loose bed over 1 m in, everything carried onto them out) is the likely cause. Switching the coast off also changes the Region, so a cell-by-cell difference of the two mosaics is not a seam measure (mean 22 m everywhere).
- **Waves.** Deep-water growth already matched SMB (100 km at 15 m/s: 2.44 m). In shallow water deep-water growth plus shallow-water bed friction gave 0.63 m (sand) / 0.36 m (rock) for 20 km of 3.2 m water at 20 m/s, against 0.83 m by SPM and about 1 m measured on Balaton. Now the SPM curve (0.84 m), with friction only on a swell higher than the water holds. The heights "at the shore" in the survey are the depth-limited ones of the waterline cell (p50 0.25 m on 64 m cells whatever the law); the survey now also reports waves 5–15 m deep offshore.
- **Supply.** Rivers bring 1.0–1.7·10⁴ m³/yr of sediment (45 % sand, 15 % gravel) into standing water over the 9 tiles, 33–54 m³/yr per km of shore: 100–320 m³ per metre of shore over the 6 kyr the coast works. Cliff waste is negligible there (0–0.4 m³/m: a lowland coast). Loose cover on the dry shore cells: 22 m³/m with the coast off, 61 with the old waves, 120–160 with the new. The waves carry 3–6 times more onto Rim and Settled cells (`left`, 4–7·10⁸ m³) than they receive (`entered`, 1–2·10⁸ m³) and than the rivers bring in the stand time: thin beaches are a supply-and-boundary matter, not one of waves alone.
- **Offshore waves, old law → new** (`.out/coast/waves3/`, the tree of 11:00): sea1 Local 2 × 2, 5–15 m of water, p50 0.65 → 0.80 m, p95 0.90 → 1.10 m; Region 2 × 2 at X 4980736 Z −6291456, p50 0.50 → 0.70 m, p95 1.10 → 1.30 m, highest 1.85 → 2.00 m. Region wave-cut platforms widen: p50 95 → 398 m, p95 478 → 1024 m (a cell), on 24.6 → 64.5 km of rock shore — bench waves now stay at what the bench depth holds instead of being worn below it.
- **Side effect of the new law on the land.** Same 2 × 2 Local tiles of sea1: sea shore 543 km with the old waves, 391 km with the final law (153 km with a first version that let deep-water growth run on over the shelf, since fixed). Low islands of loose ground are planed down where the waves are now higher on the shallow Region shelves (`.out/coast/waves2/coast-sea1-{old,new2}-L2.png`). Not judged against a reference yet.

Earlier, seed 42 `terra coast --find` (find2, find3): no lake with a real beach (Region lakes break up at Local), sea1 a lowland coast of islands and bays with thin sand fringes and offshore bars, cliff2's highest "cliff" a gorge wall at a tile seam. Pictures in `.out/coast/find2/`, `.out/coast/find3/`.

## 4. Next steps

1. Drift at Settled cells: hand in what the neighbour's shore would pass (e.g. the load leaving the adjacent Free cell in the previous sweep, a zero-gradient boundary) instead of capacity × loose bed, and take back no more than that; then measure again whether the waves still lay more next to tile edges (`seams.cpp` probe, section 7).
2. Judge the land lost to the new waves on shallow Region shelves (sea1 543 → 391 km of shore) and the wider Region platforms (p50 398 m) against a reference: wave planation of loose lowland is real, but `Carry`/`Stir` are per sweep, not per year (section 8), so the Region's many steps may overdo it.
3. A lake with a real beach (H1) is not findable on seed 42: Region lakes shallower than 45 m are levelled at Local (`levelHollows`), now the level-consistency stream's. Re-run `--find` when that lands.
4. Re-run `terra coast --find 1` with the new waves and report pictures.

## 5. Decisions and why

- **One hook after `cutAndFill`.** Rivers deliver first, then waves rework what they brought. The face above a wave-cut foot is not failed by coast code: the next step's `breakSlopes`/`settleDebris` (landscape) do that.
- **Waves as eight trains** (one per neighbour direction), state = H² in m². Carried cell to cell downwind (`sweepDownwind`), grown along the fetch-limited curve of the water's depth (SPM 1984 eq. 3-39: H = L·tanh(H_deep/L), L the highest sea the depth holds under that wind, H_deep the SMB deep-water growth `FetchGrowth·U²` per metre; each cell re-enters the curve at its own depth from the deep-water height that stands for the fetch behind), capped at the fully developed sea, cut by depth (`H ≤ 0.5·depth at high water`). A sea higher than L (swell from deeper water) loses height to the bed (`dH/dx = −0.0106·H²/h²` times the intermediate-depth share of linear theory for a wind sea of steepness 0.04; a fifth on a loose bed) down to L, not below. Rejected: deep-water growth plus friction everywhere (it double-counts the bed losses SPM's shallow curves already contain and gave shallow lakes 60 % of measured heights). Land lets nothing through, so shelter and exposure by the angle of the coast to the wind need no extra rule; a 20 % blend from each side stands for refraction and directional spread. Rejected: ray tracing per shore cell (cost, and no natural boundary from a parent level).
- **Wind**: every quarter gets `StormGain·(|W|·cos² + StormCalm)`; trains are not weighted by frequency.
- **Still water** (`spreadWater`): breadth-first from cells the flood left wet, through every cell covered at high water (`level + swing`), plus the land cells next to them. `swing` is `SpringTide` (1 m) for the sea and 0.25 m for a lake. `Shore::stood` is captured once per step because `Domain::level` of a cell the coast lowers is stale (it would read as a lake at the old cliff top).
- **Rock**: threshold `Hc = 0.063·√(MPa·sound·(1 − 0.85·weakness)/quality)`, rate `0.02·(H − Hc)/Hc²` m/yr per train, averaged over the eight. `Shore::notch` is the cut-back of a column's foot; at cell width the column comes down to the bench (`still − ½·swing`, Region 2 m deeper). Self-limiting: a bench makes the waves break (`Breaker`) and lose height to its bed, so retreat stops at a width set by rock and waves, independent of the number of steps. At coarse levels the bench narrower than a cell lives in `notch` (sub-cell platform width). Time per step is `min(step, StandTime = 6 kyr)`, cut in `CutSlices = 8` slices; a face cut through before its share of time is spent comes down and hands the rest of the time to the ground it laid bare (waves raised again between slices), so a weak face goes back by several cells in a step and a strong one by none.
- **Mud** of a fallen column is suspended after the waves are raised again: the cloud walks down the bed (across a level bench, away from where it rose), sinks a share only in calm water, settles whole in a calm hollow and otherwise leaves the domain (`CoastLedger.left`). Rejected: laying it at the first local minimum, which buried the new bench.
- **Sea caves** only where the face is being cut (`notch ≥ NotchLeast`), on master joints (spacing 8–60 m), as long as `min(CaveLongest, CaveReach·ln(struck/withstand), faster·notch)`, where `faster` is how much faster the crushed joint goes back than the face. Rejected: a hash share of joints (a stamp).
- **Standing water** is judged against `Domain::surface` (the ground of the flood), and `spreadWater` does not cross into dry ground lower than the water's low stand (beyond its sill): a channel cut after the flood is not a lake, and a lake's level does not run down its outlet.
- **Debris** of a fallen column goes to `Shore::pending` (surf debris, not in the height field), blocks ground down by the rock's `abrasion` over `SurfTravel`. Rejected: putting it on the cell as cover — a 60 m column would leave a 50 m rubble pile, while in nature the debris is removed far faster than a face crosses a cell.
- **Sand**: per train, a load is carried downwind through cells within reach (closure depth 2.5·H); capacity `Carry·E^1.25·reach`; excess is laid (not above high water), deficit is picked up (`Stir` per sweep); a class may not climb a slope steeper than it holds (`SwashSlope / √(1 + depth/0.5)`: gravel 0.27, sand 0.08, fines 0.004), the blocked part is turned to the two cells 45° to either side — that is longshore drift; at the shore the swash lays sand then gravel up to the run-up height. Undertow moves a class back down where it lies steeper than it holds; fines only ever move that way. Rejected: pairwise slope relaxation (anti-diffusive for slopes below equilibrium, one cell per round), and a full CEM shoreline tracer (too large).
- **`Domain::trend` is moved by what waves cut and lay** (in `book`, `Shore.cpp`). Without it a finer level's found uplift (`landscape/fn/Uplift.cpp`) would heal a bench or a spit back onto the parent's surface. Approved by the lead.
- **`recordShore`** raises the final waves on a working copy: the domain it gets is the caller's, and every copy of a `Domain` shares its `Shore`.
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

The coast switched off, for comparisons, without a flag in shared code: compile `Shore.cpp` with `-DworkShores=workShoresOn` and `Seas.cpp` with `-DreadSeas=readSeasOn` and link a stub that defines both empty. Seams per region of a mosaic: `seamsOf` (`measure/fn/Seams.hpp`) reads dry cells only; for the shore band and for `ShoreRecord::laid` by distance to the tile edge use a scratch driver over `gatherMosaic` and `world.atlas.find`. A scratch link must use a library copy built from the same headers as the objects linked before it, or it crashes on layout differences.

Fast loop without touching the shared build: compile `src/coast/fn/*.cpp` (and `tool/*.cpp` for terra) into a scratch directory and link them before a copy of `libderivean-simulation.a`; the coast tests need a `main` calling `testCoast()` linked with `test/Coast.cpp test/CoastVoid.cpp test/LandscapeFixture.cpp`.

## 8. Open questions

- Wave spread (`WaveSpread`, 20 % to each side per cell) is a per-cell blend, so the lateral spread per metre depends on the cell size (independent review, 26 Nov). A directional spread in angle would need the trains' energy to be moved by distance, not by cells.
- The rock threshold `Hc ∝ √(strength)` is a calibration in Sunamura's form, not his linear one; it compresses the chalk-to-granite contrast to about 5× (review). Kept because the rates it gives (chalk decimetres a year, granite nothing) match the catalogue; to be revisited with measured retreat rates.
- `WaveQuantum` caps recorded heights at 12.75 m; a fully developed sea under a 10 m/s mean wind is higher. Only the Region's open rims can reach it.
- `Carry` and `Stir` are per sweep, not per year: the drift of a level follows its step count. Spits grow slowly; a Region with many steps planes loose lowland faster than its calendar says.
- Spit test: waves square to the shore carry 0.26 of what oblique ones carry into the bay (bound 0.3): their ±45° trains, a quarter as strong, find sand only on the western side. A test with sand on both sides, or one that compares directions of drift, would tell square from weakly oblique waves better (review).
- Drift handed in at the open rim and by Settled cells stands for the shore beyond as it is (capacity × loose bed over 1 m); it is booked in `CoastLedger.entered`, not in `TileLedger.sedimentIn` (which is the rivers'). Across tiles this is a boundary condition, not conserved mass.
- Glacial low stands of the sea are not in the finer levels' sea level; drowned valleys exist only where the Region's dated sea level rose.
- Survey: `platformWidth` takes the shortest platform run over a shore cell's wet sides, which along a shore-parallel strip can read kilometres (the "widest" figure); p50/p95 are the trustworthy ones.
