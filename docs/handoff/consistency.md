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


**Seams on the tree at 6af424b** (`probe5/seams`, all 12 seams of a 3 × 3 block; test world, seed 42 at 1.2 Gyr):

| block | rivers handed on, lost (water) | step rms, seams / interior lines | groove mean, seams / interior | band lake share, seams / interior |
| --- | --- | --- | --- | --- |
| Local test block (907, 585) | 21, 0 | 78.6 / 18.4 m | −25.0 / −13.3 m | 0.47 / 0.03 % |
| Local (911, 585) | 20, 5 (51 %) | 27.7 / 20.6 m | −18.5 / −14.5 m | 3.99 / 2.12 % |
| Region (56, 36) | 18, 5 (44 %) | 85.4 / 42.5 m | −85.2 / −39.8 m | |
| Region (58, 38), mostly sea | 3, 0 | 37.7 / 51.6 m | −95.2 / −35.8 m | |

Every lost river "ran back": the later tile's water reaches a finished cell whose record sends it straight back, or through a chain of finished cells back into the later tile, where nothing carries it. The shared test checks only the seams of two tiles of the test block, so it passes or fails with where those rivers happen to cross.

**Which side is finished decides the seam.** The order tiles are visited in does not matter: block (911, 585) built with all threads asking the finest tile first and again with one thread asking every dependency last first is bit-identical (`terra measure determinism --seed 42 --age 1.2e9 --x 14934016 --z 9592832 --finest 2 --order-level 2 --pass 3`). Which tile of a seam is finished is fixed by `passOf`, a parity of the key. That parity is arbitrary, and the outcome depends on it heavily. With the Local passes shifted by one tile on both axes (`cons/sim38`, every Local seam finished from the other side), the seam crossings of the same blocks agree with the normal build for 61 % (test block) and 45 % ((911, 585)) of their water within one parent cell; the lone replays agree 100 %, as they must. The test block's 1.6 × 10⁹ m³/yr river crosses seam (907, 586)|(908, 586) at 214 with the normal passes and carries 2.8 × 10⁷ there with them flipped. Block (911, 585) loses 3 of 11 rivers (19 %) flipped against 5 of 20 (51 %), and its band lake share is 5.62 % against 3.99 %. A world whose seams change with an arbitrary parity is the defect behind the losses, the lakes and the step: two tiles simulate the same overlap, put the valley or the outlet in different places, and the one built first decides.

**Seam crossings against the parent** (`probe5/crossings`; the parent's course is the Region tiles for a Local block; a crossing is a parent cell whose way leaves across the seam with at least the child's river threshold; "carried" is child crossing water of the same direction within one parent cell, 16 Local cells):

| block | parent crossings (same river again) | finished tile alone | later tile alone | finished built | later built |
| --- | --- | --- | --- | --- | --- |
| test block | 93 (7) | 85 % | 92 % | 55 % | 67 % |
| (911, 585) | 103 (13) | 67 % | 60 % | 67 % | 79 % |

Each tile replayed alone puts most of the parent's crossing water within one parent cell (mean offset about 6 cells); the misses are a few large rivers, and there each tile puts the crossing where the other one would not. The parent's crossing matches neither tile reliably, but it is the one place both can be told. Parent rivers recross the same seam in 7–13 % of crossings (a river along the seam); lone tiles split one parent crossing over several in 2–11 of 43.

At Region the parent's course is `Backdrop::drainage`, and it is no authority yet. On (56, 36) it crosses the 12 seams 32 times, mostly as runs of adjacent cells in the same direction (12 side by side on one seam, 1.2–2.8 × 10⁸ m³/yr each): its parallel channels. The Region tiles cross 67–127 times, and 95–100 % of their crossing water crosses where the drainage has none; they carry 19–59 % of the drainage's crossing water within one drainage cell.

**A-lite2, measured and not landed** (`cons/sim36`: base plus Flood deferring and Flow refusing a finished cell whose record leads back into the domain, followed through chains of finished cells, `Domain::leadsBack`; `adoptNeighbours` moved to `fn/Neighbours.cpp`):

| block | rivers lost (water), base / A-lite2 | step rms | seam-made lake, deepest body |
| --- | --- | --- | --- |
| test block | 0/21 / 0/21 | 78.6 / 83.9 m | 0.92 %, 215 m / 0.42 %, 88 m |
| (911, 585) | 5/20 (51 %) / 0/17 | 27.7 / 38.5 m | 3.22 %, 204 m / 6.69 %, 149 m |
| Region (56, 36) | 5/18 (44 %) / 0/21 | 85 / 142 m | |
| Region (58, 38) | 0/3 / 0/2 | 37.7 / 39.1 m | |

It removes every loss but forces the water along the seam. The added step sits in rows where the later tile now carries a river in its four seam cells, mostly in the edge cell (65 % of the added squared step at (911, 585), 70 % at Region): a valley with one bank of finished ground that cannot erode. On the lake-rich block it doubles the seam lake: a basin whose lowest outlet is a finished cell that hands its water straight back fills to a higher spill (an 11 087-cell lake at (911, 585) standing 107 m above the lone replay's). Each tile put that basin's outlet in the other tile. Determinism (the closure at focus 0, 0 at 4.6 Gyr, all threads against one thread asking last first: bit-identical) and the tile cost (31.3 / 32.6 s and 48.7 / 49.2 s base / A-lite2, run side by side) are unchanged.

Seam-made bodies are artefacts of that disagreement, not lakes the ground should hold. The deepest A-lite2 body of the test block (88 m, base 215 m, (908, 585) row 0) is held by the later tile's own ground; the lone replay cuts a valley from the same entry cell 300–440 m lower 2–11 cells in, the built tile sends that water east along the seam and never cuts it. Uplift is the same in both (built 502–567 m, alone 502–587 m). With relief's `followFinished` switched off (`cons/sim37`) the body goes but the block's seam lake rises to 0.94 % with a 182 m body elsewhere and the groove to −33.8 m.

**Parent lakes at Local** (traced on the wired tree, (908, 585); the base tree has none in this block): the start surface stands 25–71 m above a 4 m parent lake (a clamp in `seedFromParent` fixes the start, `cons/sim22`); over the window `raise()` lifts the rock 167–178 m with no erosion under the lake and cover adds 9–13 m, so every cell ends above the water. `raise()` is relief's (`Uplift.cpp`); the lead routed it there.

The coast finder's lake1 (4928000, −6024704) and lake2 (4986368, −5981696) were dams of the old seam blend: they lie 4.5 and 5.5 cells from a Region tile edge, and with `keepCourses` off the Region holds them (21 km², 135 m deep; 108 km², 69 m deep), with it neither.

**Made seam rows (DREV-002, `test/SeamFixture.{hpp,cpp}`, in `test/LevelConsistency.cpp`).** The shared test read only the rivers the real relief happened to put across two tiles' seams, and on 9e96fcf plus diff it found none. It now first builds four rows of three Local tiles under a made Region parent: a V valley 150 m deep and 24 km across falling 4 m/km, with 3 × 10⁸ m³/yr entering, east along a row and south down a column, each at both parities (the row shifted by one tile has each seam finished from the other side). The river is measured across lines 128 cells apart, through each seam and inside each tile, as a share of the river entering the row. The seams' mean loss may exceed the interior's by two standard errors of the difference (interior sd for both), and no seam may lose more than two interior sd. The rows lie west of the focus because soluble rock east of it (907 and beyond) takes the river underground between the lines; a row whose rock sinks more than 1 % of the river fails as unreadable. On the tree at f17deb2 plus diff, every row's river gains on the way (no seam loses water). Seams −13.5 % against the interior's −14.7 % (sd 1.8; bound −13.1, worst seam −6.1 against 3.6) pass. A seam from a later tile into a finished one gains 3.6 points less than one the other way (−11.7 against −15.3): the finished tile's record does not take the later tile's extra water. `carriedAcross` adds a cell's whole discharge along `flowTo`, which `Extract.cpp` picks as the main share of several directions: exact for a channel, a main-direction estimate for split sheet flow; an exact balance would sum share × discharge per edge (Codex, DREV-002-02). The seed site is still measured beside it (12 rivers handed on, all carried). Its seam-lake check fails on that tree, 3.66 % against 0.76 % inside, a known failure. Log: `.out/consistency/drev002/levelconsistency-made-rows.log`.

**Gates prototype (measured 26 Nov, not landed; `cons/sim41`, patch `.out/consistency/gates2/gates-prototype.patch` sha256 0b7d6b9e…d0b1 against `cons/sim40`).** `fn/Gates.{hpp,cpp}`, `type/Gates.hpp`, `Domain::gates`, `TileLedger::gated`. findGates is at the end of seedFromParent (Local only), honourGates after shapeSlopes; the cut goes through FlowScratch::delivered. The threshold is the parent's channel head (2.5 × 10⁷). The ceiling is Domain::trend at the section's centre cell (lead's decision; with the raw parent surface the corridor cut in 193 of 198 sections, against our own blend).
- Flipped-parity agreement of the seam crossing water within one parent cell: test block 61 → 89 %, (911,585) 44 → 63 % (lone replays 100 %).
- Restated gates: lake share fails on the unflipped test block (0.35 against a bound of 0.07 %) and on (911,585) both ways; it passes on the flipped test block. Step fails everywhere (test 62.5 / 21.9 m).
- Notch: in cut sections, p50 1–1.5 m and p90 29–71 m, against p90 29–61 m in sections never cut. The worst is a V water gap 400 m through a ridge at (913,587), flipped.
- gated is under 0.1 % of erosion; cost is unchanged.
- Seam loss ("rivers handed on … lost", `probe5/seams41b`; local 0 0 / local 4 0): base 20 rivers 0 lost / 24 rivers 5 lost, 20.35 % of the water; gated 18, 0 / 23, 4, 7.54 %; base flipped 28, 1 (1.88 %) / 14, 2 (18.09 %); gated flipped 28, 1 (1.17 %) / 14, 3 (19.25 %). The gates cut the loss on unflipped (911,585) from 20 to 7.5 % but not flipped (18 against 19 %), matching the 63 % flipped agreement there: they do not close it. Determinism, block (911,585) pass 3 with gates on: bit-identical (`.out/consistency/determinism/gates-block2-pass3.txt`).
- Reviewer: cuts per section are unconnected and can refill; judge sections by Free cells only (order dependence otherwise); trend is not the pure smoothed parent; the diagonal corridor is short.
- Next: the path-following corridor (least-cost path over the corridor's cells, cut only that path), then remeasure. **On hold behind the planet** (README): face-edge seams (planet slice S5) are where this work continues, with explicit seam ownership (Codex: `passOf` parity fails on face seams).

## 3. In progress: the parent as the authority on seam crossings (design, lead's decision)

Not code yet. The finished tile must not decide where a river crosses or a basin drains, because which tile is finished is an arbitrary parity; the parent is built before both tiles of any seam and is the same whichever way the tiles are visited. The parent already decides where rivers cross a domain's rim (`injectInflow`, `fn/Seed.cpp`: inflow at the crossing and the rim outlets); the design extends that to the tile edges inside the domain.

- **The record.** The parent's own course: `Tile::flowTo` and `Tile::discharge` of the parent tiles (the Region's for a Local tile, the Local's for a Site tile) and `Backdrop::drainage` for a Region tile (8.2 km, 8 Region cells). A gate is a parent cell whose way leaves across the child tile's edge with at least the child's river threshold (`riverThreshold`); it lies on the edge within one parent cell (16 Local cells, 8 Region cells). It depends on the parent and the edge alone, so both tiles of a seam find the same gates, whichever is built first.
- **How a tile honours it: carve, never bar.** Every tile, whatever its pass, takes the gates on all four of its edges. The corridor is the child cells under the parent's course from one parent cell upstream of the edge to one downstream. Its ceiling is the parent's surface along that course, interpolated between parent cell centres (the parent's water where its course runs through its lake). After each step, in each cross-section of the corridor, if the lowest Free cell stands above the ceiling it is lowered to it, and the rock or cover taken goes into the load like any erosion. That forces one notch, not a trench a parent cell wide. Where the child's own valley already runs below the parent's river, nothing changes. The flood needs no deferral: the corridor gives the basin an outlet at or below the parent's river at the gate.
- **At a seam.** The finished tile honoured the same gate when it was built, so its cells in the corridor stand at or below the ceiling and its record crosses there; `settledWay` hands that river on at the gate as today. The later tile's own water reaches the gate through its own corridor instead of a finished cell that would send it back. No bars: A-lite and A-lite2 are dropped. Re-crossings (a parent river along the seam) are gates too, each in its direction.
- **Where the parent has no course across the seam** (rivers below the child's threshold at parent scale, the sea, a parent lake whose outlet is not on the edge): no gate, and today's handling stays. A parent lake keeps its level through `Domain::lakeFloor` on both sides. What is left is small rivers the parent cannot place; the restated gates below bound what they may do.
- **Ownership.** A new consistency-owned `fn/Gates.cpp` finds the gates at build (`buildDomain`, after `seedFromParent`) and clamps the corridor after the erosion of each step (`Replay.cpp`). Relief's files are not touched. The cost is a pass over the parent cells along four edges per tile and a clamp over at most 3 × 16 × 16 cells per gate a step.
- **Risks to measure in the mirror first.**
  - The Region's gates come from the drainage, whose channels lie side by side at 8.2 km (93 % of channel cells, section 5); Region gates may be too many and too parallel. On (56, 36) they are: 95–100 % of the Region tiles' crossing water crosses where the drainage has no crossing. Gates at Region wait for a drainage whose channels are single.
  - The corridor must follow the child's notch, not cut a straight line: the ceiling binds one cell per cross-section only.
  - Basins whose parent outlet is not on the edge but whose child outlet is (the (911, 585) lake): the parent's lake level through `lakeFloor` decides them, which today is only a floor; it may need to be a ceiling too.

**Restated gates for the test** (`probe5/crossings`): the seams must look like any other line through the tiles. Over the 12 seams of a block, the band lake share (8 cells either side) and the step rms each must stay within two standard errors of the same measure across the 54 interior lines of the block (lines 64, 128 and 192 of each tile, both ways): seam mean ≤ interior mean + 2 sd_interior / √12. The interior lines see the same landscape, so a lake district or a mountain block raises both; the two-standard-error threshold for a few deterministic lines is a heuristic, not a stated false-alarm rate (Codex, DREV-002-02).

| block | lake share, seams / bound | step rms, seams / bound |
| --- | --- | --- |
| test block, base | 0.47 / 0.07 % (fails) | 65.1 / 21.2 m (fails) |
| test block, A-lite2 | 0.23 / 0.08 % (fails) | 71.9 / 21.3 m (fails) |
| (911, 585), base | 3.99 / 4.31 % (passes) | 23.3 / 23.9 m (passes) |
| (911, 585), A-lite2 | 8.62 / 10.95 % (passes) | 33.7 / 22.5 m (fails) |
| (911, 585), base, passes flipped | 5.62 / 5.34 % (fails) | 36.8 / 25.7 m (fails) |
| test block, base, passes flipped | 0.99 / 1.13 % (passes) | 64.3 / 23.8 m (fails) |
| Region (56, 36), base | 1.50 / 1.36 % (fails) | 61.8 / 47.7 m (fails) |
| Region (56, 36), A-lite2 | 1.08 / 1.56 % (passes) | 95.9 / 51.9 m (fails) |

The step here is the mean of the per-seam and per-line rms values, so it differs from the pooled rms in the table above. The A-lite2 interior lake share rises too (4.96 % against 2.12 %): its Region parents changed. The test block fails both gates on every tree: its seams carry 15 times the interior's lake share and four times its step.

## 4. Next steps

1. **Path-following gates in the mirror** (cons/sim41), with the reviewer's items 1, 2, 4 and 5 fixed; acceptance: flipped agreement ≥ 90 % on both blocks, no seam loss in the made rows, restated gates, no worst cut that reads as a trench. Held behind the planet; resumes on the face-edge seams of S5. Also: relief reports `testLevelConsistency` at 1.81 % against 0.07 % on 98868ae (relief's Glacial review changes move the ratio); re-measure on the new main before tracing.
2. **The test** takes the restated gates instead of the inside-share comparison, with every river handed across all 12 seams of the test block carried on (`cons/sim31/test/LevelConsistency.cpp` has the all-seams crossing check and the seam-made measure; the deepest seam-made body is reported, not bounded). It lands with item 1.
3. **Gates at Region, step 2 of (c)** (lead, 26 Nov), once the Local gates prove out. They need an authority with single channels, and `Backdrop::drainage` runs side by side (section 2). The trunk field `routeTrunks` (`fn/Trunks.{hpp,cpp}`, `fn/TrunkFlow.{hpp,cpp}`, `type/TrunkRouting.hpp`, `type/Trunks.hpp`, not wired) has single channels: side-by-side rivers 1.7 % against 93 %, Hack 0.63–0.65. It stays in the tree as the candidate and leaves only if it does not serve as Region's gate authority.
4. Region step: the seam step at Region is twice the interior's on the land block; re-measure after item 1.

## 5. Decisions and why

- **Water gaps follow Earth** (owner, 26 Nov about 20:00: "we copy Earth's behaviour; whenever our reference implementation, the real Earth, has the pattern, use it"). An antecedent gorge through a ridge stays, as on Earth where a river held its course while the ridge rose. The corridor must still only keep the parent's course open; the gorge's depth must come from the river's own erosion, not from the cut. At (913,587) the base needed 22 m of cut and the river deepened the rest, which fits.
- **The parent is the authority on seam crossings, not the tile built first** (lead, 26 Nov): which tile is built first is an arbitrary parity, and the seams follow it (section 2). The parent is the same in any order.
- **A-lite2 does not land.** It removes the losses by barring the finished cells that send water back, which forces the water along the seam (a one-sided valley, the step) and fills basins whose outlet each tile put in the other (seam lake doubled on (911, 585)). The same bar limited to cells that hand straight back (A-lite) has the same lakes.
- **B does not land** (the later tile's edge cells track the finished tile's prediction of them, `nowWide`, faded by `seamWeight`): at Region the finished tile's prediction of the later tile's ground differs from that tile's own by kilometres over 4 cells; the step rms rose from 75 to about 300 m with every partner, and at Local it lost the same rivers.
- **The trunk field is dropped as the authority on Region rim crossings** (in the tree since cef4340, never wired; it stays as the candidate for Region gates, section 4). It routed the planet's rivers at 4.1 km over the window's start with roughness, the Region's uplift and rebound, in 17–22 s per world with workers. Its network matched Earth better than the drainage (side by side 1.7 % against 93 %, Hack 0.63), but it did not match the Region's own trunks (1 of 35 crossings above 3 × 10⁹ within 8 cells). Wired as the authority on Region rim crossings it made the seam losses worse with every partner (B + wiring 10.7 % at Local, A-lite + wiring 26 % at Region) and added 16–30 s to `readBackdrop`. The Local seams of a block lie inside one Region tile, so it could not fix them in any case. `Backdrop::drainage` stays.
- **Conservation does not explain the groove**: a finished cell passing on its record plus what reached it beyond what its record assumed (C) left the groove mean the same or worse at both levels and lost more at Region (31 %).
- **The later tile takes a finished neighbour's river as inflow at the cell the neighbour sent it to**, not where its own ground would: the finished cells do not erode, so a river that meets them as a dam is lost to both tiles.
- **Rim prediction cannot fix rivers into a finished tile.** Budgets agree (Region runoff against drainage yield 0.84–1.36 per tile; the lost catchments get 0.8–0.9 of their water from the drainage); places do not. Routing the Region's start surface finds 22 of 136 final trunks above 3 × 10⁹ within 8 cells (water-weighted 0.23), the drainage 11 (0.09): the trunks form during settling. Rejected alternatives: gateways a finished tile exposes (amounts still disagree, outcome depends on build order), upstream-first build order (unbounded dependency chains).
- **A blend never raises ground across a course either side drains through** (water decides first). A channel-only rule (no raise where discharge ≥ channel head) halved the lost Region crossings but kept the dams of smaller valleys; following every course removes them.
- **The parent-lake floor is the parent's water, not its sill.** In a dry climate the parent's lake is limited by evaporation; the floor follows its water and the child's lake can be no higher.
- **A lake floor is not carried to a parent cell lower than the lake beside it**, the channel below an outlet or a lower lake's shore, or it would protect the wrong lake there.
- **A finer level keeps a basin only under parent water at least 1 m deep** (`LeastLake`, `fn/Seed.cpp`).
- **Drainage is a value of the backdrop**, solved once per world (36 MB, 0.8 s with eight threads, bit-identical).

## 6. Contracts with other streams

- Relief and groundwater read `Domain::lakeFloor` (empty in a region; −1e9 where no parent lake bears on a cell). `seamWeight` (`landscape/fn/Build.hpp`) is the one seam weight; the volcanic and aeolian seam blends use it, so the three edits land together.
- Seam labels and the volcanic pile still blend with the full weight where `keepCourses` took the ground's blend back.
- Groundwater's `spillHollows` (`Flow.cpp`) treats water reaching a finished neighbour's cell as gone (`overEdge`), as the route does; gates change where that happens, not the rule.
- Relief's `followFinished` blends the uplift of Free cells within 64 Local cells of a finished neighbour; gates clamp the ground, not the uplift, so the two do not overlap.

## 7. How to verify

Private mirrors under the scratchpad `cons/`, each a copy of `@simulation` with its own `XMAKE_CONFIGDIR` and build directory (`build<N>.sh`); a probe is built with `c++ -std=c++20 -O2 -Isim<N>/src -Isim<N>/test probe5/<p>.cpp sim<N>/test/LandscapeFixture.cpp build<N>/macosx/arm64/release/libderivean-simulation.a`.

- `probe5/seams <local|region> dx dz [rows.tsv]`: the 3 × 3 block (moved by dx, dz tiles from the test block, or its Region tile), all 12 seams: LOST lines with the path, the seam profile (groove, step) against interior lines, a tile's replay time; rows.tsv holds every seam row for `cons/steprows.py base.tsv variant.tsv threshold` (where a variant's step grows). Outputs in `.out/consistency/step/`.
- `probe5/crossings <local|region> dx dz`: parent, built and lone crossings of every seam, their agreement, and the restated gates; `cons/flipcmp.py a.txt b.txt 16` compares two builds' crossings. Outputs in `.out/consistency/cross/`.
- `probe5/seamadded test [dx dz]`: seam-made lakes per tile with their bodies (`cons/pool.py` pools them; its band sizes assume the normal passes). `probe5/seambody tx tz cx cz`: one body, its sill on both sides, the finished sill's record, and the way out built and alone.
- Mirrors: `sim35` the tree at 6af424b with the new test; `sim36` that plus A-lite2, `fn/Neighbours.cpp` and the trunk field removed (the A-lite2 landing, kept for reference); `sim37` `sim36` with `followFinished` off; `sim38` `sim35` with the Local passes shifted by one tile. Older mirrors (`sim19`–`sim30`, B, C, wiring) are superseded; their patches are in `cons/patch/`.
- Determinism: `terra measure determinism --finest 2 --order-level 2 --pass 3` (test block closure, `.out/consistency/determinism/`); add `--seed 42 --age 1.2e9 --x 14934016 --z 9592832` for (911, 585).
- Wider checks: `terra measure seams|levels`, `simulation-tests consistency`.

## 8. Open questions

1. Region gates: the drainage's parallel channels may give a Region seam many weak gates. Does a Region tile need gates only above a larger threshold, or the drainage fixed first?
2. A basin whose outlet the parent puts inside one tile but each child puts across the seam: is the parent's lake level enough, or must the gate carry the parent's sill too?
