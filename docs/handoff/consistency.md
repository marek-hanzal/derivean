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

The coast finder's lake1 (4928000, −6024704) and lake2 (4986368, −5981696) were dams of the old seam blend: they lie 4.5 and 5.5 cells from a Region tile edge, and with `keepCourses` off the Region holds them (21 km², 135 m deep; 108 km², 69 m deep), with it neither. The Site tiles at lake2 are bit-identical whether built alone or after the 4 × 4 block around them in reverse (`.out/consistency/order`): no build-order dependence.

**Trunk field** (`landscape/type/Trunks.hpp`, `fn/Trunks.cpp`, `routeTrunks`; not yet called): the planet's rivers at PlanetCell / 16 (4.1 km, 4096² cells) over `pastElevation` with the Region's seeded roughness and rock quality, the planet's own yield shared over each planet cell's land, a priority flood, and the region window's erosion by stream power with the levels' exponent (one rock, no uplift) in 12 implicit steps. Deterministic with and without a worker pool. 55–80 s and about 0.2 GB per world under load.

| network, channels above 1000 km² (100 km²) | side by side | sinuosity p50 | Hack | Rb | density km/km² |
| --- | --- | --- | --- | --- | --- |
| `Backdrop::drainage` (8.2 km, smooth) | 93 % (98 %) | 1.04 | 0.46 (0.97) | 20, 6.9 (27, 6.8) | 0.065 (0.158) |
| trunk field | 2.5 % (19 %) | 1.26 | 0.60 (0.63) | 5.5, 6.3 (5.5, 5.2, 5.7) | 0.026 (0.070) |
| Earth | – | about 1.1–1.5 at this scale | 0.5–0.6 | 3–5 | about 0.027 (0.085), from HydroRIVERS' 36 million km at 10 km² scaled by A^−½ |

"Side by side": channel cells with a parallel channel of at least a third of their water beside them, which is the parallel-filament defect. The roughness removes most of it; the erosion tightens the network. Rb at the low orders stays 5.2–5.7 at either threshold, slightly above Earth's. Against the Region's own trunks on the 3 × 3 from (18, −25) the field places 3 of 31 edge-crossing trunks above 3 × 10⁹ within 8 Region cells with 0.7–1.5 of their water (water-weighted 0.13; drainage 0.22): the Region's settled trunks follow neither, which is what the wiring is to change.

## 3. In progress

- Nothing running.

## 4. Next steps

1. **Wire the trunk field** (decided: option A of the item 1 options) once the lead says relief's Region erosion change has landed: Region rim inflow and outlets at trunk crossings with the trunk's water, replacing `Backdrop::drainage` (one owner for the fact) and its call in `readBackdrop`. Before that:
   - sample roughness and quality at the crust's material position as the Region does (`materialOf`, `strata/fn/Column.cpp` drift), and average them over the 4 × 4 Region cells of a trunk cell: `roughnessAt` has octaves down to 1.1 km, finer than the 4.1 km grid can hold;
   - give a planet cell whose land all falls under the sea after roughness its yield somewhere (the drainage drops it too);
   - cut the cost (the flood seeds every sea cell; 13 floods of 16.8 M cells), or solve it once per world off the start path;
   - decide whether the region tiles' own trunks must also conform inside a tile, not only at its rim; measure with the position check above.
2. Deep Region lakes in the later tile's first three cells (x edges 9 / 5 / 2 %, z edges 17 / 14 / 10 %, 130–340 m deep): rivers of a later tile that meet a finished tile's settled ground and pond. Item 1; expected to fall with the trunk routing.
3. At Local the uplift blocks straddling a tile edge (relief, `Uplift.cpp`) left the later tile's edge column lower than its neighbours; relief's `followFinished` addresses it. Re-measure the edge band (`lakeprobe` `band` lines) after it settles.

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

Private mirrors under the scratchpad `cons/`, each a copy of the shared tree with its own `XMAKE_CONFIGDIR` and build directory (`build4.sh`, `build5.sh`); `mk4.sh` builds a probe against the mirror's library.

- `probe4/onetest/onedbg`: `testLevelConsistency` alone with one line per crossing.
- `probe4/regionseam 5241344 -6100000 3 <out> 6`: Region seams, lake share, crossings, and `band` lines (lake, river, along-edge river share and depth by signed distance from the inner edges, positive into the later tile).
- `probe4/lakeprobe 4712960 -6459904 2 0 <out> 0 4`: a Local 2 × 2 block from the Region; parent lakes (`[L2 start]` is the child's start), seams, bands, levels, crossings.
- Outputs: `.out/consistency/t4` (mirror of the tree before relief's floor diff, with and without `keepCourses` and the floor), `t5` (the shared tree), `budget`, `approach` (item 1 analysis).
- Wider checks: `terra measure seams|levels|determinism`, the suite.

## 8. Open questions

1. Does the trunk routing settle rivers into a finished tile at Local too, or does Local need its own authority (the Region's rivers decide crossings there already, yet 57 % of the into-earlier crossings at the lowland block lose water downstream)?
