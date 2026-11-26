# Hand-over: coast

Describes the present state of the `coast` workstream. Rewrite it when the state changes; it is not a diary.

## 1. Goal

Own `@simulation/src/coast/**` (+ `test/Coast*.cpp`, `tool/Coast*.cpp`): shores of seas and lakes that arise from simulated waves working on what the land offers, at the Region, Local and Site levels. Wanted outcomes, checked against `docs/reference-landscapes.md` (G3, G4, H1, H2, F2): cliffs with a platform at their foot; coves, headlands, stacks, arches and sea caves along joints; beaches sorted by grain size; spits, barriers, lagoons, tombolos; a lake with a sandy beach findable on seed 42. Nothing placed, nothing painted by noise.

## 2. State now

Written, **not yet compiled or run**:

- `coast/constant/Waves.hpp` — every coefficient of the wave model and the retreat law.
- `coast/type/Shore.hpp` — `Shore` (working state of a replay, hangs off `Domain::shore`), `ShoreRecord` (kept by a finished tile, `Tile::shore`), `CoastLedger`, `sweepDownwind`.
- `coast/type/SeaVoid.hpp` — contract of the volumetric query (declared only).
- `coast/fn/Coast.hpp` — declarations of the domain's functions.
- `coast/fn/Waves.cpp` — `stormWind`, `raiseWaves`.
- `coast/fn/Shore.cpp` — `shoreOf`, `spreadWater`, `workShores` (the step hook), booking into the tile ledger.
- `coast/fn/Cliff.cpp` — `withstandOf`, `cutCliffs`.

Nothing is verified. Nothing is hooked into the step loop yet.

## 3. In progress

Writing `coast/fn/Drift.cpp` (`moveSand`). Then `coast/fn/Seas.cpp` (`readSeas`, `recordShore`), hand-built scenarios, first compile.

Shared-file edits already made: the three lines in `landscape/type/Terrain.hpp` listed in section 6. The other hooks are **not** in yet.

## 4. Next steps

1. `Drift.cpp`: sand and gravel by train, as designed in section 5.
2. `Seas.cpp`: open boundary from the planet (Region) or the parent's `ShoreRecord` (Local, Site); `recordShore`.
3. `coast/fn/Scenario.{hpp,cpp}`: hand-built domains (straight rock coast with a river mouth; bay between headlands; lake), built like `measure/fn/PlaneDomain.cpp` on `allocateBare`.
4. Compile the coast files alone against the existing static library (section 7), look at pictures of the scenarios, tune.
5. Add the remaining hooks (section 6), build with mise, run the whole test suite.
6. `test/Coast.cpp` (+ two lines in `test/Main.cpp`), `tool/CoastCommand.cpp` (+ one line each in `tool/main.cpp`, `tool/Arguments.hpp`).
7. `coast/fn/SeaVoid.cpp`; wire it next to `caveSpans` in `simulation/fn/Block.cpp` / `relief/fn/Form.cpp` (the ground agent has finished; the lead asked this stream to do it); extend the block-alignment test with a point inside a sea void.
8. Real world, seed 42: find coasts and lakes by search (`terra find|scout`, `terra coast`), look at `terra view` pictures against the catalogue's photographs, measure tile time.
9. Independent reviewer sub-agent; rows for `docs/simulation.md` to the lead; final report.

## 5. Decisions and why

- **One hook after `cutAndFill`.** Rivers deliver first, then waves rework what they brought. The face above a wave-cut foot is not failed by coast code: the next step's `breakSlopes`/`settleDebris` (landscape) do that.
- **Waves as eight trains** (one per neighbour direction), state = H² in m². Carried cell to cell downwind (`sweepDownwind`), grown by fetch (SMB: `FetchGrowth·U²` per metre, capped at the fully developed sea), cut by depth (`H ≤ 0.5·depth at high water`) and by bed friction (`dH/dx = −0.0106·H²/h²`). Land lets nothing through, so shelter and exposure by the angle of the coast to the wind need no extra rule; a 20 % blend from each side stands for refraction and directional spread. Rejected: ray tracing per shore cell (cost, and no natural boundary from a parent level).
- **Wind**: every quarter gets `StormGain·(|W|·cos² + StormCalm)`; trains are not weighted by frequency.
- **Still water** (`spreadWater`): breadth-first from cells the flood left wet, through every cell covered at high water (`level + swing`), plus the land cells next to them. `swing` is `SpringTide` (1 m) for the sea and 0.25 m for a lake. `Shore::stood` is captured once per step because `Domain::level` of a cell the coast lowers is stale (it would read as a lake at the old cliff top).
- **Rock**: threshold `Hc = 0.063·√(MPa·sound·(1 − 0.85·weakness)/quality)`, rate `0.02·(H − Hc)/Hc²` m/yr per train, averaged over the eight. `Shore::notch` is the cut-back of a column's foot; at cell width the column comes down to the bench (`still − ½·swing`, Region 2 m deeper). Self-limiting: a bench makes the waves break (`Breaker`) and lose height (`BedFriction`), so retreat stops at a width set by rock and waves, independent of the number of steps. At coarse levels the bench narrower than a cell lives in `notch` (sub-cell platform width). Time per step is `min(step, StandTime = 6 kyr)`, sliced so that at most one cell width is cut per round.
- **Debris** of a fallen column goes to `Shore::pending` (surf debris, not in the height field), blocks ground down by the rock's `abrasion` over `SurfTravel`. Rejected: putting it on the cell as cover — a 60 m column would leave a 50 m rubble pile, while in nature the debris is removed far faster than a face crosses a cell.
- **Sand** (to be written): per train, a load is carried downwind through cells within reach (closure depth 2.5·H); capacity `Carry·E^1.25·reach`; excess is laid (not above high water), deficit is picked up (`Stir` per sweep); a class may not climb a slope steeper than it holds (`SwashSlope / √(1 + depth/0.5)`: gravel 0.27, sand 0.08, fines 0.004), the blocked part is turned to the two cells 45° to either side — that is longshore drift; at the shore the swash lays sand then gravel up to the run-up height. Undertow moves a class back down where it lies steeper than it holds; fines only ever move that way. Rejected: pairwise slope relaxation (anti-diffusive for slopes below equilibrium, one cell per round), and a full CEM shoreline tracer (too large).
- **`Domain::trend` is moved by what waves cut and lay** (in `book`, `Shore.cpp`). Without it a finer level's found uplift (`landscape/fn/Uplift.cpp`) would heal a bench or a spit back onto the parent's surface. Approved by the lead.
- **Traps**: every border cell of a domain must be `Rim` (`raiseWaves` reads three neighbours of every other cell). `Domain` copies share one `Shore` (`shared_ptr`). `workShores` returns before allocating anything when no water stands.

## 6. Contracts with other streams

Volumetric query (`coast/type/SeaVoid.hpp`), approved by the lead:

```cpp
struct SeaVoid { double ax, az, bx, bz; float y, wide, high, water; SeaVoidKind kind; };
struct SeaVoids { std::vector<SeaVoid> voids; };
inline constexpr double SeaVoidReach = 40.0;
SeaVoids seaVoidsOver(TileReader& site, const Setting& setting, double minX, double minZ, double maxX, double maxZ);
int  seaVoidSpans(const SeaVoids&, Vec2 position, CaveSpan* spans, int capacity);
bool inSeaVoid(const SeaVoids&, double x, double y, double z);
```

Lines in shared files — **in**:

- `landscape/type/Terrain.hpp`: `struct Shore;` and `struct ShoreRecord;` after `struct GroundDetail;`; `std::shared_ptr<const ShoreRecord> shore;` in `Tile` after `detail`; `std::shared_ptr<Shore> shore;` in `Domain` after `TileLedger ledger;`.

Lines in shared files — **still to add** (approved):

- `landscape/fn/Process.hpp`: `void workShores(Domain& domain, FlowScratch& scratch);`
- `landscape/fn/Replay.cpp`: `workShores(domain, scratch);` after `cutAndFill(domain, scratch);` in `stepDomain`.
- `landscape/fn/Build.cpp`: include `coast/fn/Coast.hpp`; `readSeas(domain, setting, lookup);` in `buildDomain`.
- `landscape/fn/Extract.cpp`: include; `tile.shore = recordShore(domain);` in `extractTile`.
- `test/Main.cpp`, `tool/main.cpp`, `tool/Arguments.hpp`: declaration and dispatch of the coast test and command.
- `simulation/fn/Block.cpp` / `relief/fn/Form.cpp`: sea void spans next to cave spans.

Other owners: landscape and its stability rules ("relief" agent), `rockmass`/`strata`/`planet` (lead), `measure`, `groundwater`, wind and dunes (separate agent; dry beach sand above high water is ordinary Sand cover for it).

## 7. How to build, run and verify

```sh
mise -C @simulation run build     # library, terra, tests (build directory is shared with other agents)
mise -C @simulation run test
.out/simulation/build/macosx/arm64/release/terra <command> ...
```

Fast loop for coast files only, without touching the shared build (objects go to `.out/coast/`):

```sh
clang++ -std=c++20 -O2 -I@simulation/src @simulation/src/coast/fn/*.cpp <driver>.cpp \
  .out/simulation/build/macosx/arm64/release/libderivean-simulation.a -o .out/coast/dev
```

## 8. Open questions

- Whether barriers and spits emerge above water with the capacity rule as designed, or need a tuned `Carry`/`Stir`; to be settled on the bay scenario.
- Cost per tile of the rounds (target: well under a fifth of a coastal tile's time; inland tiles pay one scan).
- Glacial low stands of the sea are not in the levels' sea level; drowned valleys exist only where the Region's dated sea level rose. To be stated as absent unless the landscape owner wants a low-stand phase.
