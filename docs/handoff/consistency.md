# Hand-over: level consistency (lakes, rivers, tile seams)

Describes the present state of the level-consistency stream. Rewrite it when the state changes; history belongs in Git.

## 1. Goal

Lakes, rivers and tile seams are the same features at every level and across every tile edge:

1. A lake the parent level has stays a lake, with its level and shore, at the finer levels, unless a simulated process of the finer level (infill, outlet incision) removes it, and then the process is named. The owner wants a lake with a sandy beach that can be found (`terra coast --find 1`).
2. A river the parent has enters a fine tile where the parent's river crosses the tile's edge, with the parent's discharge, and leaves where the parent's leaves. Region rivers keep their water across Region tile edges.
3. No steps or straight lines along tile edges (Local: coast, ice, relief all report them).
4. Lake cover at Region against Earth (about 1.5–3 % of land, more on glaciated shields): measured after 1–3, judged per landscape type.

Owned: `@simulation/src/landscape/**` files the relief stream is not changing (not `fn/{Uplift,Fluvial,Stability,Talus,Hillslope,Glacial}.cpp`, `Process.hpp`), `@simulation/test/LevelConsistency.cpp`. Fixes in relief-owned files go to the lead as diffs.

## 2. State now

In the shared tree (uncommitted, this stream):

- **Rivers handed on across tile edges.** A finished neighbour's cell whose recorded flow leaves into a cell the later tile evolves hands its recorded water and load to that cell as `inflow` (`Domain::settledWay`, `adoptNeighbours` in `fn/Build.cpp`). That cell may not drain back into it (`fn/Flood.cpp` defers the step, `fn/Flow.cpp` excludes it) and passes the river on through evolving cells while it has a way down among them. Before, the later tile replaced every finished cell's discharge by the neighbour's record, so a river its own ground led along the finished strip was dropped and its valley ended at a wall (Region dump: a lake at 1382 m against a 700 m step).
- **Region rim inflow from a finer planet drainage.** `Backdrop::drainage` (`type/Drainage.hpp`, `fn/Drainage.cpp`, solved once in `readBackdrop`) routes the planet's rivers eight times finer (8.2 km) over `pastElevation`; `seedFromPlanet` injects rim inflow from it instead of the 65 km planet grid, so a river enters near where the region's own valleys take it and at the same place for both regions of an edge.
- **Parent lakes kept at the start of a finer level.** `seedFromParent` fills `Domain::standing` at every finer level; `levelHollows` no longer fills cells under the parent's standing water (it filled basins by up to 45 m at Local and 6 m at Site, parent lakes included, and made that the trend).
- Test `testLevelConsistency` (`@simulation/test/LevelConsistency.cpp`): rivers the two finished Local tiles beside a later one send into it are still carried four cells down its course.

Measured (seed 42, 4.6 Gyr; mirror `sim2` of the shared tree before the groundwater hooks patch; `before` = the same tree without this change):

| | before | after |
| --- | --- | --- |
| Region 3 × 3 from (18, −25): crossings losing > 30 % within 2 cells | 33 % (into earlier 22 %, later 40 %) | 11.5 % (into earlier 37 %, later 0 %) |
| same, followed four cells down the receiving tile's course | – | into earlier 32 %, later 1 % |
| Region lakes within 3 cells of an edge against inside | 20× | 13× |
| Region lake share of land | 0.97 % | 0.86 % |
| Local 2 × 2 at (4712960, −6459904), mirror `sim` variants on the old Region: parent lake cells that are lake at Local | 68 % (start 70 %), 8.5 m deep under a 43.8 m parent lake | 83 % (start 93 %), 28.6 m deep |
| same: parent rivers found within one parent cell | 64 % | 100 % |
| same: crossings losing > 30 % | 38 % (into later 64 %) | 10 % (into later 0 %) |
| determinism, Local pass-3 tile and its closure, two worlds, threads and order | – | bit-identical (`.out/consistency/port-det`) |

With the new Region drainage the same Local block has a different, shallower parent lake (15 cells, 12.7 m): 65 % of it is lake at the Local start, 23 % after settling, 4.2 m deep with the level 2.4 m above the parent's. The level holds and the area shrinks, so the process that removes it is infill by the Local rivers' sediment over the 6 Myr settling; the parent keeps it as open water.

Region lake share is about 1 % of land with this measure (`Mosaic::lake`). Earth's lakes cover about 1.5–3 % of land, more on glaciated shields (Canada, Finland): low but not absurd, to be judged per landscape type. A third of the Region lake bodies still lie mostly within 3 cells of a tile edge.

Coast finder (`terra coast --find 1` from (5172398, −5980190), `.out/consistency/port-coast`): before this change all four lakes it offered lay on Region tile edges (lake3 and lake4 at X ≈ 5241000, the edge at 5242880). Now lake1 (4928000, −6024704; 14.7 km², 116 m deep, surface 516 m) lies inside a Region tile, lake2 about ten cells from an edge, lake3 still on an edge. At Site, lake1 has 69 % sand shore with beaches 12–80 m wide and spits; the shore view (`view-lake1-sand-shore.png`) shows a sand spit and low dunes behind the water. It is not yet Balaton or the Michigan dunes: the sand at the waterline stands at 24° (p50) where a real beach face is 2–10°, there is no berm or swash zone, and the far shore shows terraced stripes.

## 3. In progress

- Nothing running.
- The Site pattern the ice stream reported (`.out/icesheet/site2/local-off.png`) is reproduced at (7042816, 13263360) (`.out/consistency/sitepat/site.png`). It is not seeded noise: material coordinates and the roughness are world-attached (`probe/material.cpp`), the start state carries no sediment (Local cover is under 1 m on 99.8 % of cells), and the fine detail does not repeat (high-pass correlation between tiles 0.01–0.13). What repeats is where sediment ends up: every tile ends with 1–2 m of loose cover in its eastern half and almost none in its western half, and the wind there blows from the west (2.7 m/s, `windX` > 0, `windZ` ≈ 0). Each Site tile piles its own sand on its downwind side, as if no sand crossed its upwind edge. That points at the aeolian pass that ends a Local or Site tile (`aeolian/`), not at the landscape seed: for the aeolian stream.

## 4. Next steps

In the lead's order:

1. **Rivers into an already-finished tile** (a third still lost at Region). Evidence (`.out/consistency/port-region/run.txt`, `LOST` lines with the later pass first): the big ones are losses of amount, not of place. A river of 1.8–2 × 10¹⁰ m³/yr leaving a pass-2 tile northwards finds 0.2–0.3 of that within eight cells in the pass-0 tile (`within8`). The finished tile carried only what its own halo gathered plus its rim inflow from `Backdrop::drainage`; the later tile's own catchment gives three to five times more. Next: compare a Region tile's own runoff per area with the drainage yield it replaces (the planet's discharge spread evenly over its fine cells) on the same catchment; if the budgets differ, the fix is there, otherwise in where the drainage puts the crossing.
2. **Straight rivers and rows of small lakes along Local tile edges** (`.out/consistency/lowlake/local-v0.png`, x ≈ 256 and z ≈ 256, before and after this change).
3. **Deep Region lakes beside tile edges** (13× denser than inside; hundreds of metres deep at 3000–4500 m): into-earlier rivers meet finished cells that rose during the replay. Likely shares a cause with 1.
4. **With the groundwater hooks in** (`.out/consistency/karst-region`, `karst-lowlake`, same blocks): Region crossings losing > 30 % 10.4 % (into earlier 39 % within 2 cells, 21 % followed downstream; into later 0 %), lakes near edges 11.7× (was 13×), lake share 0.83 % of land. Local block: the parent lake (14 cells, 13.8 m) is 69 % lake at the start but 17 % after settling, 0.2 m deep with the level 4.5 m below the parent's: it now drains (the hooks' outlet rule for hollows whose floor takes the water) rather than filling in; whether its floor is soluble rock there, and whether the lake flips between lake and open from step to step, is not checked. Local lakes near edges 2.5× (was 10×); parent rivers found 98 %; crossings into a later tile 0 % lost, into an earlier one 29 %.
5. At Local a chain of lakes forms along the parent's main river during settling where the parent has none (`.out/consistency/port-lowlake/local-v0.png`, row ≈ 390; also before this change). Suspect: the relief rule that keeps a cell gathering the parent's channel head from cutting below the parent's surface (`Fluvial.cpp`).

## 5. Decisions and why

- **The later tile takes a finished neighbour's river as inflow at the cell the neighbour sent it to**, not where its own ground would: the finished cells do not erode, so a river that meets them as a dam is lost to both tiles. Water a finished neighbour simulated in its halo and that also forms in the later tile can be counted twice where a river zigzags across an edge; accepted, it is rare and bounded by one crossing's flow.
- **Bilinear parent rock near parent lakes was tried and rejected**: with lakes kept from `levelHollows` it kept fewer parent lake cells at Local (75 % against 83 %).
- **Carving the parent's rivers into the child seed was tried and rejected**: no gain in rivers found or crossings kept; keeping lakes and handing rivers on already finds 100 % of the parent's rivers at the lowland block.
- **Drainage is a value of the backdrop**, solved once per world, not a cache: its identity is the planet's, and the world owns it. Its ground is sampled over the world's workers (`readBackdrop(planet, age, workers)`, `World.cpp`): 0.8 s with eight threads against 1.8 s on one, bit-identical; the rest is the priority flood. 36 MB per world.
- **A finer level keeps a basin only under parent water at least 1 m deep** (`LeastLake`, `fn/Seed.cpp`): a pit the parent's flood barely filled is a hollow of its surface, and keeping its whole block of child cells from levelling would leave the interpolation hollows as lakes.
- **A river handed on keeps out of the finished neighbour's cells while it has another way down**, in the cell it enters and every cell below it (`fn/Flow.cpp`): a finished cell passes on its own record, so the river would be dropped one cell further on.

## 6. Contracts with other streams

- Relief (`Fluvial.cpp`, owned by relief): a Settled cell with `Domain::handsOnward(i)` passes no water; its load enters as `inflowLoad` at the cell it hands to. Diff for the lead, in `carryLoad` after the Settled load override:

  ```diff
           load[Fines] = 0.40f * domain.settledLoad[i];
       }
  +    // A finished neighbour's cell that hands its river on has its load entered as inflowLoad
  +    // where it hands it: it neither passes nor books any here.
  +    if (domain.handsOnward(i)) continue;
  ```

  Without it, the final sweep books such a cell's load in `ledger.sedimentOut` (and in the plume when the cell is flooded), so `terra relief` totals of sediment leaving a tile are too high; the ground itself is unaffected. A follow-up there: the deposit-room cap is built from `share`, which a handing-on cell has none of, so the cell it feeds may aggrade above it.
- Every Region tile changes (rim inflow from `Backdrop::drainage`), and every finer tile through it: baselines taken before this change must be re-taken.
- `Domain::standing` is now filled at Local and Site too (water surface the parent holds over a cell, sea included); only `levelHollows` and the ground level read it.

## 7. How to verify

Private mirrors (scratchpad `cons/`): `sim` is the tree with `labVariant` switches (bit 1 keep lakes, 2 bilinear near lakes, 4 carve rivers, 8 hand-on, 16 drainage); `sim2` is a copy of the shared tree, built by `build2.sh` with its own `XMAKE_CONFIGDIR`.

- `probe2/onetest` runs `testLevelConsistency` alone.
- `probe2/regionseam 5241344 -6100000 3 <out> 3`: Region seams, lake share, crossings (near-edge and downstream), lake bodies with their distance to an edge.
- `probe2/lakeprobe 4712960 -6459904 2 0 <out> 1 3`: a Local 2 × 2 block and a Site 2 × 2 block replayed from the Region; parent lakes under the block (`[L2 start]` is the child's start state), seams, levels, crossings; pictures with the parent's lakes outlined and rivers dotted in red.
- Outputs: `.out/consistency/port-region`, `port-lowlake` (after), `region-base`, `lowlake` (before, variants 0/8/9/11).
- Wider checks: `terra measure seams|levels|determinism`, the suite.

## 8. Open questions

1. Are the deep Region lakes beside tile edges (hundreds of metres deep at 3000–4500 m) acceptable until rivers into an earlier tile are solved, or should the finished tile's edge cells be lowered where a later tile's river arrives?
