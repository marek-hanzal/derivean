# Hand-over: level consistency (lakes, rivers, tile seams)

Describes the present state of the level-consistency stream. Rewrite it when the state changes; history belongs in Git.

## 1. Goal

Lakes, rivers and tile seams are the same features at every level and across every tile edge:

1. A lake the parent level has stays a lake, with its level and shore, at the finer levels, unless a simulated process of the finer level (infill, outlet incision) removes it, and then the process is named. The owner wants a lake with a sandy beach that can be found (`terra coast --find 1`).
2. A river the parent has enters a fine tile where the parent's river crosses the tile's edge, with the parent's discharge, and leaves where the parent's leaves. Region rivers keep their water across Region tile edges. The coarser level is the authority on where water goes and how much, so outcomes do not depend on build order or the player's path.
3. No steps, straight lines or rows of lakes along tile edges.
4. Lake cover at Region against Earth (about 1.5–3 % of land, more on glaciated shields): measured after 1–3, judged per landscape type.

Owned: `@simulation/src/landscape/**` files the relief stream is not changing (not `fn/{Uplift,Fluvial,Stability,Talus,Hillslope,Glacial}.cpp`, `Process.hpp`), `@simulation/test/LevelConsistency.cpp`. Fixes in relief-owned files go to the lead as diffs.

## 2. State now

In the tree (this stream; earlier parts committed):

- **Rivers handed on across tile edges.** A finished neighbour's cell whose recorded flow leaves into a cell the later tile evolves hands its recorded water and load to that cell as `inflow` (`Domain::settledWay`, `adoptNeighbours` in `fn/Build.cpp`); the cell is not reached from that neighbour in the flood (`fn/Flood.cpp`) and the river keeps to evolving cells while it has a way down among them (`fn/Flow.cpp`). Relief's rule in `routeWater`: a Settled cell passes nothing down the slope into an evolving cell, only what it hands on.
- **Region rim inflow from a finer planet drainage** (`Backdrop::drainage`, 8.2 km, `fn/Drainage.cpp`).
- **Parent lakes kept.** `seedFromParent` fills `Domain::standing` (parent water over a cell) and `Domain::lakeFloor`: the parent's water level over its lake cells and over the parent cells beside a lake whose ground stands at that water or above (its rim and outlet). Erosion (`Fluvial.cpp`, relief), slides and solution (`Process.hpp` `rimAbove`) and karst (`groundwater/fn/Sinking.cpp`) do not lower a cell below it. The lake fills from the child's own water, so it may stand lower than the parent's, never spill lower.
- **`levelHollows`** (`fn/Build.cpp`) judges a hollow whole, over every cell its water stands over (halo included), and levels it only if it is no deeper than the level's artefact limit (Local 45 m, Site 6 m, Ground 0.4 m), lies above sea level and holds no parent water. Deeper basins keep their full depth.
- **Water-first seam blend.** `blendSeams` fades the edge cells toward the finished neighbours' records (`seamWeight`, `fn/Build.hpp`, shared with the volcanic and aeolian seam blends); `keepCourses` (`fn/Extract.cpp`) then takes the blend back wherever it would dam or dig a course: down the steepest way of the tile's own ground, and along every river a neighbour hands on, a blended cell stays no higher than the cells draining into it and no lower than the cell it drains to.
- Test `testLevelConsistency`: every river the finished Local tiles around a pass-3 tile (and around the pass-1 tile beside the focus) send into it is followed four cells down the later tile's course and must keep 70 % of its water; a step back across the seam counts what the finished record carries there (nothing, if that finished cell sends its own water into the later tile). The pass-3 tile's lake share within three cells of its edges must stay under three times its inside share plus 1 %.

Measured, seed 42, 4.6 Gyr:

| | before | now |
| --- | --- | --- |
| Local test tile (pass 3): lakes within 3 cells of its edges / inside | 11.5 % / 0.18 % (blend, before relief's settled rule) | 0.96 % / 0.43 % |
| same, relief's settled rule in, blend without `keepCourses` | 1.88 % / 0.23 % | – |
| Local 2 × 2 at (4712960, −6459904): parent lake (34–37 cells, 15 m) kept as lake at Local | 0.5 %, level 17.7 m below the parent's (drains) | 46.5 %, level +0.3 m, 3.7 m deep |
| same: parent rivers found within one parent cell | 73 % | 98 % |
| Region 3 × 3 from (18, −25): lake share of land | 1.21 % | 0.71 % |
| same: lakes within 3 cells of an edge / further inside | 6.2 % / 0.88 % | 5.0 % / 0.46 % |
| same: later-side lakes 3–7 cells from an x edge | 4.6–9.4 % | 0.7–1.5 % |
| same: crossings losing > 30 % (into earlier, followed downstream) | 10.6 % (27 %) | 9.8 % (23 %); into later 0 % |

Region lakes are 0.71 % of land against Earth's 1.5–3 %. The lakes `keepCourses` removed were dams of the blend. The gap is partly the glacial lake districts and partly the closed arid basins the planet does not have yet (README, Defects 6).

**Known failure:** the test's crossing check fails on one crossing, kept as it is until the trunk routing (section 4) lands; it runs last in the suite (`test/Main.cpp`), so the tests before it are still checked. Local tile (907, 584) takes a river of 9.2 × 10⁷ m³/yr from its pass-0 western neighbour at z ≈ 233; it joins the later tile's own river of 4.3 × 10⁸, which runs back west into the finished tile, where the record carries 1.4 × 10⁵. It predates this stream's changes (the same loss with them switched off). It is item 1 at Local: two tiles that simulated the same overlap put the river in different places, and the one finished first decides.

The loss has only moved on the uncommitted tree after a913f5b, it is not fixed. Bisected on mirrors (`.out/consistency/handon`): HEAD with only the volcanic changes (and the new materials) or only the coast changes no longer has it, HEAD with only groundwater, ice sheet or `Glacial.cpp` still does. Both change the ground upstream, so that river no longer crosses there (volcanic: a 3.1 × 10⁸ river enters at z = 224, carried whole; coast: no crossing on that seam). On the combined tree the pass-3 tile instead loses two rivers across its south seam (q 2.9 × 10⁶ and 6.3 × 10⁶, carried 0). The acceptance for item 1 is that no later tile's river runs back into a finished one at any seam of the 3 × 3.

The test world is seed 42 at 1.2 Gyr (`testWorld`), the other measurements are at 4.6 Gyr.

The coast finder's lake1 (4928000, −6024704) and lake2 (4986368, −5981696) were dams of the old seam blend: they lie 4.5 and 5.5 cells from a Region tile edge, and with `keepCourses` off the Region holds them (21 km², 135 m deep; 108 km², 69 m deep), with it neither. The Site tiles at lake2 are bit-identical whether built alone or after the 4 × 4 block around them in reverse (`.out/consistency/order`): no build-order dependence. The Local closure around (4712960, −6459904) with `keepCourses` (9 Local and 9 Region tiles, all threads against one thread asking last first) is bit-identical too (`terra measure determinism --finest 2 --order-level 2 --pass 3`, `.out/consistency/determinism`).

**Seam-made lakes** (`probe5/seamadded`, mirror only; proposed to replace the test's seam-lake check, which compares the edge band with the tile's inside and fails or passes with the inside share). Each tile of passes 1–3 of a 3 × 3 is replayed a second time with no neighbours of its level (its parents only, as a pass-0 tile is built: only `adoptNeighbours` reads same-level tiles; wind, seas, volcanic and ice read parents and accept a missing tile). A lake cell of the built tile that the lone replay does not hold, and that no parent lake covers, is seam-made. Band: cells within 8 of a finished neighbour. Pooled over the five (Local) or seven (Region) tiles:

| | band seam-made | deepest seam-made body at a seam |
| --- | --- | --- |
| Local test block, a913f5b | 1.20 % | 159 m (150 cells) |
| Local, tree at 14:25 | 0.74 % | 253 m |
| Local, same without `keepCourses` | 2.45 % | 333 m |
| Region 3 × 3 from (17, −24), tree at 14:25 | 1.09 % | 1002 m |
| same without `keepCourses` | 2.10 % | 973 m |

Without `keepCourses` the measure flags both lake1 (tile (18, −23), body at (204, 4), 9 cells, 98 m) and lake2 (tile (19, −23), body at (9, 34), 194 cells, 48 m); with it neither is there. Of the near-seam lakes relief's lake caps raised on the 14:25 tree, the largest (57 cells at the pass-3 tile's seam, 41 m) lies under a Region lake for 46 cells; the deep ones (121, 184, 253 m) lie under none. What remains are the rivers of a later tile that meet a finished tile's ground and pond (item 1).

Pass-0 tiles and lone replays hold almost no lakes at Local (0–0.03 %), and where a Region lake covers 1024 Local cells of the block, the built tiles hold 47 of them, lone replays none: parent lakes are not reaching Local here (goal 1), and the shallow interior lakes of later tiles (a 760-cell body 11 cells in, 14 m deep) exist only with neighbours.

**Trunk field** (`landscape/type/Trunks.hpp`, `fn/Trunks.cpp`, `routeTrunks`; not yet called). In the tree (9c30423): the planet's rivers at PlanetCell / 16 (4.1 km, 4096² cells) over `pastElevation` with roughness and quality at the world position, a flood seeded with every sea cell, and the window's erosion with one rock and no uplift; 59–77 s per world.

In the private mirror `cons/sim6` (not in the tree, awaiting the lead):
- roughness and quality read in the crust's frame (`forcing.strata.material` plus the offset, as `materialOf`) and averaged over the 4 × 4 Region cells of a trunk cell;
- a planet land cell with no land here yields at its highest cell (10.7 % of the planet's land yield was dropped); a planet sea cell yields only the rain on its land cells here (its ocean rain was spread over them, 6.5 times what falls there);
- the flood starts at the coast only (71 k cells instead of 13.8 M);
- the window's rock uplift as a Region gets it (`forcingAt` tectonic + residual at every trunk cell, 1.1 µs a call; from the planet grid the uplift is off by 437 m rms against a 916 m spread, from 8 km by 222 m) and the isostatic answer to erosion (`IsostaticSink` times the cut, two box passes matching the Region's spread, radius 3), the sea and coast found anew after each of the 12 steps at that step's sea level (`seaLevelAt`). The field deposits nothing, so its basins do not sink under fill.
- The water over the ground (`fn/TrunkFlow.{hpp,cpp}`, `type/TrunkRouting.hpp`) is split from the sampling and the window (`fn/Trunks.cpp`, 170 lines).

Bit-identical with and without a worker pool. 17–22 s per world with ten workers under a load average of 30 (sample 7.4 s, flood 7.4 s, erosion 2 s), 62–75 s alone; about 0.6 GB at peak.

| network, channels above 1000 km² (100 km²) | side by side | sinuosity p50 | Hack | Rb | density km/km² |
| --- | --- | --- | --- | --- | --- |
| `Backdrop::drainage` (8.2 km, smooth) | 93 % (98 %) | 1.04 | 0.46 (0.97) | 20, 6.9 (27, 6.8) | 0.065 (0.158) |
| trunk field in the tree | 2.4 % (19 %) | 1.26 | 0.60 (0.63) | 5.3, 5.8 (5.6, 5.1, 5.2) | 0.028 (0.070) |
| crust frame, averaging, yield, coastal flood | 2.7 % (21 %) | 1.23 | 0.59 (0.61) | 5.4, 5.9 (5.6, 5.0, 5.5) | 0.028 (0.071) |
| with uplift and rebound (mirror) | 1.7 % (14 %) | 1.23 | 0.63 (0.65) | 5.2, 5.9 (5.8, 5.0, 5.5) | 0.026 (0.063) |
| Earth | – | about 1.1–1.5 at this scale | 0.5–0.6 | 3–5 | about 0.027 (0.085), from HydroRIVERS' 36 million km at 10 km² scaled by A^−½ |

"Side by side": channel cells with a parallel channel of at least a third of their water beside them, which is the parallel-filament defect. Rb at the low orders stays 5–6, slightly above Earth's; Hack with the uplift is at the top of Earth's range.

Against the Region's own trunks (diagnostic, not acceptance: the field decides and the Region is to conform), 3 × 3 from (18, −25), 35 edge-crossing trunks above 3 × 10⁹, the field holding 0.7–1.5 of their water within 8 (24) Region cells:

| | within 8 | within 24 | Region minus field ground | local relief rms at 4 km, Region 281 m | correlation with the Region's |
| --- | --- | --- | --- | --- | --- |
| field in the tree | 3 (0.11) | 6 (0.61) | +1401 m | 4.3 m | 0.006 |
| crust frame … coastal flood | 7 (0.13) | 9 (0.61) | +1401 m | 4.3 m | 0.005 |
| with uplift | 1 (0.10) | 10 (0.66) | −462 m | 259 m | 0.002 |
| drainage | 2 (0.05) | 3 (0.50) | | | |

Without the uplift the field routed over the window's start, a surface 1.4 km lower and nearly flat at 4 km. The forcing's uplift alone correlates 0.18 with the Region's relief; with it the field has the Region's amount of relief but its own valleys: two networks cut independently over the same uplift do not put their valleys in the same 4 km cells, so the Region must take the field's crossings (wiring) rather than be predicted by it. The field cuts 462 m less than the Region (one rock, no hillslopes, no ice).

## 3. In progress

- The trunk field with uplift is in the tree, not wired (`fn/Trunks.cpp`, `fn/TrunkFlow.{hpp,cpp}`, `type/TrunkRouting.hpp`, `trunkNeighbour` in `type/Trunks.hpp`, `routeTrunks(const Setting&, Workers*)`; nothing calls it yet). A fresh copy of the tree builds the library, `terra` and `simulation-tests`, and gives the mirror's metrics. Wiring (item 1) and the seam-made check (item 2) are next, in the mirror; the file list goes to the lead first.

## 4. Next steps

1. **Wire the trunk field** (decided: option A; the field decides, the Region conforms; no longer waiting for relief's Region erosion change): Region rim inflow and outlets at trunk crossings with the trunk's water, replacing `Backdrop::drainage` (one owner for the fact) and its call in `readBackdrop`. Acceptance: network metrics, determinism, under 20 s with workers, and no later tile's river running back into a finished one at any seam of the 3 × 3. Open: whether the region tiles' own trunks must also conform inside a tile, not only at its rim.
2. **Seam-lake check**: replace the inside-share comparison with the seam-made measure above once the lead approves a cap (proposed: pooled band share of seam-made lake under 0.5 % and no seam-made body touching a seam deeper than the level's artefact limit, 45 m at Local). Every tree measured fails it today; it is to pass with item 1.
3. **Parent lakes at Local**: trace why the test block holds 47 of 1024 parent-lake cells, and why a lone (pass-0) replay holds none.
4. Deep Region lakes in the later tile's first three cells (x edges 9 / 5 / 2 %, z edges 17 / 14 / 10 %, 130–340 m deep): rivers of a later tile that meet a finished tile's settled ground and pond. Item 1; expected to fall with the trunk routing.
5. At Local the uplift blocks straddling a tile edge (relief, `Uplift.cpp`) left the later tile's edge column lower than its neighbours; relief's `followFinished` addresses it. Re-measure the edge band (`lakeprobe` `band` lines) after it settles.

## 5. Decisions and why

- **The later tile takes a finished neighbour's river as inflow at the cell the neighbour sent it to**, not where its own ground would: the finished cells do not erode, so a river that meets them as a dam is lost to both tiles.
- **Rim prediction cannot fix rivers into a finished tile.** Budgets agree (Region runoff against drainage yield 0.84–1.36 per tile; the lost catchments get 0.8–0.9 of their water from the drainage); places do not. Routing the Region's start surface finds 22 of 136 final trunks above 3 × 10⁹ within 8 cells (water-weighted 0.23), the drainage 11 (0.09): the trunks form during settling. Rejected alternatives: gateways a finished tile exposes (amounts still disagree, outcome depends on build order), upstream-first build order (unbounded dependency chains).
- **A blend never raises ground across a course either side drains through** (water decides first). A channel-only rule (no raise where discharge ≥ channel head) halved the lost Region crossings but kept the dams of smaller valleys; following every course removes them.
- **The parent-lake floor is the parent's water, not its sill.** In a dry climate the parent's lake is limited by evaporation; the floor follows its water and the child's lake can be no higher.
- **A lake floor is not carried to a parent cell lower than the lake beside it**, the channel below an outlet or a lower lake's shore, or it would protect the wrong lake there.
- **Handed-in rivers with no way down among evolving cells**: making the entry a hollow that fills to its lowest free spill (Flood.cpp) was tried and changed nothing at the failing crossing; that river is lost further down, where the later tile's own river returns into the finished one.
- **A finer level keeps a basin only under parent water at least 1 m deep** (`LeastLake`, `fn/Seed.cpp`).
- **Drainage is a value of the backdrop**, solved once per world (36 MB, 0.8 s with eight threads, bit-identical).

## 6. Contracts with other streams

- Relief and groundwater read `Domain::lakeFloor` (empty in a region; −1e9 where no parent lake bears on a cell). `seamWeight` (`landscape/fn/Build.hpp`) is the one seam weight; the volcanic and aeolian seam blends use it, so the three edits land together.
- Seam labels and the volcanic pile still blend with the full weight where `keepCourses` took the ground's blend back.
- Every Region tile changed with `keepCourses`, and every finer tile through it: baselines taken before must be re-taken.

## 7. How to verify

Private mirrors under the scratchpad `cons/`, each a copy of the shared tree with its own `XMAKE_CONFIGDIR` and build directory (`build<N>.sh`); a probe is built with `c++ -std=c++20 -O2 -Isim<N>/src [-Isim<N>/test …/LandscapeFixture.cpp] probe5/<p>.cpp build<N>/…/libderivean-simulation.a`.

- `probe4/onetest/onedbg`: `testLevelConsistency` alone with one line per crossing.
- `probe4/regionseam 5241344 -6100000 3 <out> 6`: Region seams, lake share, crossings, and `band` lines (lake, river, along-edge river share and depth by signed distance from the inner edges, positive into the later tile).
- `probe4/lakeprobe 4712960 -6459904 2 0 <out> 0 4`: a Local 2 × 2 block from the Region; parent lakes (`[L2 start]` is the child's start), seams, bands, levels, crossings.
- Outputs: `.out/consistency/t4` (mirror of the tree before relief's floor diff, with and without `keepCourses` and the floor), `t5` (the shared tree), `budget`, `approach` (item 1 analysis).
- `probe5/trunkfield <threads>`: `routeTrunks` with and without workers (identity, timing), flood agreement, network metrics at 1000 and 100 km² (`.out/consistency/trunks/field*.txt`).
- `probe5/regionmatch <threads> 18 -25 3` and `probe5/upliftmatch`: the field and drainage against a Region mosaic's crossing trunks and valleys (`region-*.txt`, `uplift.txt`); `probe5/forcingcost`: `forcingAt` cost and interpolation error.
- `probe5/seamadded test` (the test's block) or `seamadded <level> <x> <z> <age>`: seam-made lakes per tile, bodies with depth, centre and parent-lake cover (`.out/consistency/seams`).
- Mirrors `cons/sim6` (tree at 14:25 with the new field, `LAB_NOCOURSES` switch), `sim7` (9c30423), `sim8` (a913f5b), `sim9`–`sim14` (a913f5b plus one group of uncommitted changes each, for the 907,584 bisect, `.out/consistency/handon`).
- Wider checks: `terra measure seams|levels|determinism`, the suite.

## 8. Open questions

1. Does the trunk routing settle rivers into a finished tile at Local too, or does Local need its own authority (the Region's rivers decide crossings there already, yet 57 % of the into-earlier crossings at the lowland block lose water downstream)?
