# Hand-over: ice sheets (`@simulation/src/icesheet`)

Describes the present state. Rewrite it when the state changes; history belongs in Git.

## 1. Goal

The owner's verdict: the world looks the same everywhere. A third of Earth's land was under ice at the last glacial maximum and still shows it. The simulation has valley glaciers at Local and Site (`landscape/fn/Glacial.cpp`) and had nothing at continental scale.

This stream owns `@simulation/src/icesheet/**` (+ `test/IceSheet*.cpp`, `tool/IceSheet*.cpp`). It must produce, from simulated processes and never as props or painted noise:

1. **The sheet**: accumulates where the model's climate gives a positive balance, flows under its own surface slope (shallow ice), ends where melt or the sea takes it; extent, thickness, flow direction, thawed or frozen bed; the crust sinks under it and rebounds.
2. **Rock**: little erosion on frozen uplands, troughs along fast outlets, overdeepened below sea level and drowned as fjords with sills; on shields rock basins along joints and fracture zones that become lakes the water routing must find.
3. **Deposits**: till sheets, drumlin fields, end moraines, kettled hummocky ground, eskers along subglacial water routes, outwash plains.
4. **Levels**: Region carries the sheet and the big forms; Local the drumlins, eskers, kettles and lake basins; Site their slopes and material.

Catalogue targets are in `docs/reference-landscapes.md` (A2, A3, F1–F4, G1). Required tests and tool statistics are in section 4.

## 2. State now

**Works, with tests in the suite (`test/IceSheet.cpp`, `test/IceSheetLocal.cpp`, registered in `test/Main.cpp` before `testLandscapePlane`; both pass in the private mirror (section 7), 3 s and under 1 s; the full suite was not run this round):**

- `fn/Flow.cpp` — shallow-ice relaxation on any square grid. Test: a cold plateau grows a dome kilometres thick, flat inside and steep at its margin; a warm one stays bare; pool of 4 = no pool, bit for bit.
- `fn/Continental.cpp`, `service/Glaciations.cpp`, `fn/Dating.cpp` — planet-wide stages for a world, memoised. Seed 42, age 4.6e9: five stages (present, last maximum at −2.16 °C, three colder down to −5.63 °C). Land under ice: **36.2 % now, 39.5 % at the last maximum, 44.9 % in the coldest stage** (Earth: 10 % and 32 %). 1.3–8 s once per world.
- Region pipeline (`fn/Region.cpp`, `Wear.cpp`, `Debris.cpp`, `Meltwater.cpp`, `Work.cpp`, `Sheet.cpp`), hook live in `Replay.cpp`. Tests on hand-built domains: a valley through a coastal range is cut 150 m below the sea with a sill at its mouth while the ridge beside it keeps 1 100 m and the frozen plateau is not worn; on a shield with fracture zones the zones become lakes in rock (9 % of the ground the sheet left, lake cells three times as weak as the mean), with end moraine, outwash and an esker; the ledger closes to 1e-8; pool = no pool.
- Region on real land (seed 42). Inland freed lowland at `--x 7075912 --z 13325105`: rock worn 128 m on freed ground (mean), the ground 51 m lower than without ice, of which about 30 m is crust still to come back (e^(−13/5) of the last maximum's sink): half the freed lowland, 60 m above the sea without ice, is now under the sea, as Hudson Bay or the Baltic. Coastal range at `--x 11585794 --z 4285552`: the coast is more indented, ground under present ice 340 m lower (sink and wear). Pictures `.out/icesheet/probe2/` and `probe-coast/` (`probe-diff.png`: red lowered, hatched = under the last maximum).
- **The last sheet at Local and Site** (`fn/Parent.cpp`, `Local.cpp`, `Lodge.cpp`, `Esker.cpp`, `EskerPath.cpp`, types in `type/Parent.hpp`, `type/Esker.hpp`). After a finer domain has settled, `Leave` reads the parent's record and ice through the `tiles` lookup; the cover is taken up (as much as one glaciation strips), weak rock is dug against the mean of a parent cell (up to 50 m, at Local only; a Site inherits the basins in its parent's surface), till is carried down the parent's ice surface and lodged, filling the cavity behind every rise to a line falling at 5 %, so it piles in tails behind knobs; tunnel fills are set aside from the cover before and stand as ridges after, along a course traced down the domain's own water head (`EskerPath.cpp`); a hollow in the ice surface keeps no more than till stands in, the meltwater takes the rest; under ice thinner than 100–200 m (`StagnantIce`, `PressingIce`) hollows are not filled and stay as kettles; beyond the ice the load stays as moraine no steeper than till stands (24°), and `washBeyond` spreads gravel and sand down the ground, fills lakes to their spill level and drains under the ice. `Present` puts the parent's ice surface over the domain's own ground (valleys get more). Test on a hand-built plain (`test/IceSheetLocal.cpp`, every case through `leaveChecked`: no cell may end up more than `EskerTallest` higher above all its neighbours than before): basins dug in both fracture zones, deeper along the flow; till twice as thick in the lee of each knob and drawn out along the flow; an unbroken ridge along the stream; the esker winds with a winding trough of the bed (within 1.5 cells); it carries on from, and ends on, a finished neighbour's recorded esker at the edge; it breaks where its water ponded behind a rock sill and carries on beyond; a tributary's esker reaches the trunk's; a kettle stays open under stagnant ice; a bowl in the ice surface keeps no column (the old rule left 1 548 m on one cell); ledgers close; a tile whose up-ice halo a finished neighbour owns gets the same till within 10 %; ground the sheet never reached gets no record. Real Local tiles (2 × 2 at `--x 7052800 --z 13263360`, a freed till lowland, `.out/icesheet/local4/`; `local5/` after digging moved to the rock-mass field: lakes 5.9 %, and the domain faults are dug as straight trenches with a till bank on their lee side): lakes 0.1 % → 8.3 % of the land, mean slope 1.4° → 2.3°, till 6.5 m mean, rock worn 134 m (parent); till tails and rock basins aligned with the flow (`zoom-on.png`, `local-left.png`: brown = till). Judged against F1/F3 it is not there yet, see "Broken".
- `tool/IceSheetCommand.cpp` — `terra icesheet` (stages, planet map, freed places; with `--x --z [--level] [--tiles]` a census and two maps).

**Not yet seen working:**

- Site tiles with the last sheet. They run: a 2 × 2 Site mosaic was built at `--x 7042816 --z 13263360`. With rock dug at Site too (10 m against an 8 m plucking box), the joint planes became a city-block grid of trenches: 40 % of cells steeper than 15°, lakes 32 % (`.out/icesheet/site2/local-on.png`). Digging is now Local only. The rerun (`site3/`) is in section 3.
- Kettles in the real world: the rule is tested on a rig only; no real tile near the last maximum's margin has been looked at.
- Hummocky dead-ice ground and ice blocks buried in outwash: need the retreat stage by stage, not modelled.
- The full suite on the current tree: not run this round.

**Broken or known wrong:**

- **Band at a Local tile seam: fixed.** At the finer levels `Leave` now runs after `settleSeams`, so `blendSeams` mixes two versions of the ground both tiles settled before the sheet. The sheet's change is laid on top, and it continues across the edge from the finished neighbour's recorded `passing`. Measured with `seamprobe` (`.out/icesheet/local6/seam.txt`), rows across the edge between tiles (429,808) and (429,809): the 4–5 row dip of −2.8 m is gone; at most +1.1 m remains over the 6 rows next to the edge, the size of the ±0.8 m till waves elsewhere. The right pair runs smooth (±0.8 m). The recorded load steps 21.5 → 25.0 m there, but till does not.
- **Look at Local, against F1/F3**: many lakes are one or two 64 m cells (speckle), and rock is dug in short dashes along the grid where the rock-structure field has its joints; rivers that follow fracture zones are dug 20 m deeper with a till bank on their lee side, so they read as straight leveed channels. The ice edge from the parent's 1 km cells is a straight-sided block (top right of `local-diff.png`). Real shield country has lakes of every size up to kilometres with rough rims, and till in separate streamlined hills.
- Open review items:
  - Where the cover holds less than a stretch's ridge, the ridge is lowered by a ratio of the domain's own cells (`Esker.cpp`), so two tiles can give the same stretch different heights at their seam. A ratio from parent-only quantities would fix it.
  - Digging only lowers, so the Local mean sinks a few metres below the parent's.
  - Kettles depend on thickness only; slowness (no sliding) is not used.
  - A route that clips a tile corner can take the upstream neighbour's ridge as its downstream crossing.
- Bare rock pinnacles at Site: at tile 5474 9868 (`--x 5605504 --z 10105024`) 50 cells stand more than 20 m above all neighbours, up to 76 m, without the ice pass as well (`.out/icesheet/column/site-fixed.txt`). Not this stream's; the settled replay leaves them.
- Done since: digging reads only the rock-mass field (`pluckFactor` in `Lodge.cpp`: rock kind, `weakness` from `fractureShare`, `sound` from the jointed mass's cohesion), not the seeded `quality`. Beyond the ice, the moraine surplus no longer vanishes: gravel and sand run on as outwash through `washBeyond` (shared with Region, `Meltwater.cpp`), and mud leaves. Outwash is now in the finer record.
- Region ledger (`Debris.cpp`): ice debris moving between the tile's cells and its halo is not booked as crossing, and `sedimentOut` is booked for halo cells too. `WearTotals` close (tested), but the Region `TileLedger` over owned cells probably does not. `washBeyond` books crossings correctly now. Check it with a ledger test on a Region rig.
- The lodging is excitable: wherever load enters a domain without its history (the rim, a halo with no finished neighbour) it sets off a train of till waves about 20 cells long that decays slowly. Inside the tile it is driven by the halo's 32 cells; with a finished neighbour the recorded `passing` load is taken up instead. Watch the 2 × 2 pictures for seams.
- `Continental.cpp` reports a thickest cell of 8 131 m: a deep sea-floor cell enclosed by iced cells takes the coarse surface over its own bed when the half-resolution result is handed to the full grid. Harmless for extents, wrong as a number.
- The shared build breaks whenever another stream's file is half-written; a silent failure leaves an old binary. Use the build script of section 7, which fails loudly.

## 3. In progress

Nothing half-done. Fifth run (Opus, 26 Nov, on the lead's list after 8073c64), all built and tested in the private mirror (section 7); `test/IceSheetLocal.cpp` and `test/IceSheet.cpp` pass there. The full suite was not run.

- **Columns at Site (lead, urgent): fixed.** `lodgeTill` let a cell with no lower neighbour on the parent's ice surface keep all the load passing over it. At Site tile 5474 9868 (`--x 5605504 --z 10105024`) that was till of 1 070 m, 605 m and 359 m on single cells. Now such a hollow keeps what till stands in and the meltwater takes the rest (section 5). With relief's `settleDebris` guard absent (the mirror has none), the same tile has no loose cell more than 20 m above its neighbours; the 48 cells that stand more than 20 m above all neighbours are bare rock, up to 61 m, and the tile without the pass has 50 such cells up to 76 m (`.out/icesheet/column/site.txt` before, `site-fixed.txt` after). Every Leave in the tests goes through `leaveChecked`; a bowl in the ice surface gave a 1 548 m column on the old rule.
- **Eskers follow the domain's own head** (`EskerPath.cpp`, section 5). Coast, Local 2 × 2 at `--x 11582976 --z 4307456` (`eskerstat`):

  | | Tallest | Cells over 1 m | Mean height | Runs | Width |
  |---|---|---|---|---|---|
  | Parent's route (8073c64) | 17.8 m | 570 | 5.1 m | 4 | 2.1 cells |
  | Traced, first walk (D8) | 19.3 m | 416 | 7.4 m | 4 | 2.0 cells |
  | Traced, streamline (`coast3/`) | 16.9 m | 614 | 6.3 m | 9 | 2.1 cells |
  | Final code (`coast4/`) | 18.2 m | 328 | 6.0 m | 6 | 1.9 cells |

  The D8 walk still drew straight lines in eight directions (`coast2/zoom-left.png`). The streamline courses bend with the valleys and around lakes (`coast3/zoom-left.png`); on flat lake floors they run straight down the ice surface's slope. `coast3` had one small loop near an aimed end; the walk has changed since (section 2 tests). `coast4/` is the run on the final code, but the settled ground there changed too (other streams' work came in with the mirror), so the two rows are not a like-for-like comparison: no loop, and one ridge runs straight for about 90 cells along a flat valley floor and lake, down the ice surface's slope.
- **Kettles**: no pit or shelter fill under ice thinner than 100 m, full above 200 m, in proportion between. Tested on a rig only.
- **Review items done**: `passing` by grain class; `sheetThen` (the last maximum's sheet over the tile's own ground, read by the level below instead of `iceAge`); `washBeyond` rewritten (section 5); dead `Tunnel::before/after` removed; `finishedSheet` in `Parent.cpp` reads a finished neighbour's record.
- **Seam where the ice flows into a finished neighbour** (`seamprobe2`, `.out/icesheet/seam2/seams.txt`, Local 2 × 2 at `--x 7052800 --z 13263360`, code before the column fix): three of the four seams have the ice flowing into the finished tile. The ground change by the pass (on − off) steps by at most 0.2 m between the two rows at the edge and wanders ±1 m over eight rows on either side, the size of the till waves elsewhere. No fix needed.
- **Counts against F1/F3** (`landforms.cpp`, `.out/icesheet/count/`, freed ground only, Local 64 m cells):

  | | Inland till lowland `7052800 13263360` | Coast `11582976 4307456` | F1 shield | F3 till plain |
  |---|---|---|---|---|
  | Freed share of the square | 61 % | 30 % | | |
  | Relief per km / per 10 km | 31 / 128 m | 451 m / – | 10–60 / 20–150 m | < 3–10 / 10–30 m |
  | Lakes on freed ground | 4.9 % | 0.7 % | 10–30 % | |
  | Lakes by size (1, 2–9, 10–99, 100–999 cells) | 304, 333, 153, 9; largest 3.3 km² | 39, 27, 12, 1 | 0.01–1 000 km² | |
  | Shoreline / circle (10+ cells) | 1.7 | 2.6 | 3–10 | |
  | Till hills over 2 m (mean size, height) | 231 (500 × 486 m, 5.7 m); 106 longer along the flow; 4 drumlin-shaped | 4 | | drumlins 250–1 000 m long, L/W 1.7–4.1, 5–50 m, 300–1 000 m apart |
  | Rock knobs over 2 m | 234, 3.4 m high | 339, 39 m high | 5–50 m, 50–500 m | |
  | Eskers | none (no tunnel fill there) | 9 runs, longest 7.6 km, 17 m | 10–50 m, tens to hundreds of km | |
  | Water-filled hollows in drift | 357, 2.3 m deep; 142 sized as kettles | 18, 12 sized | | 10–1 000 m, 2–40 m |

  Verdict: the inland square has F1's relief but a third of F1's lakes, mostly one to nine cells and round; its till lies in round low swells with no preferred direction, not drumlin swarms; the coast is a mountain country (fjord-like), not F1/F3. Eskers are a tenth of F1's length because one Local square holds only a stretch of the parent's tunnels.

## 4. Next steps

1. **Counts against F1/F3** are in section 3: what is missing is drumlin fields and lake share on a shield. Look for a freed shield (`terra icesheet` lists freed places) and count there; the till lowland at `7052800 13263360` is F3 country, not F1.
2. **Esker ratio per stretch from the parent alone** (open review item), then re-check the 2 × 2 seams with eskers on the coast.
3. **Kettles on a real tile** near the last maximum's margin; slowness as a second condition if they come out where the ice was fast.
4. **Region**: no recessional moraines yet. The Region `TileLedger` books halo cells (`Debris.cpp`). The 2 × 2 Region map across a tile edge is not made.
5. Bring the `column.cpp` check (cells more than 50 m above all neighbours after the pass) into `terra icesheet` so a census shows it.

## 5. Decisions and why

**One sheet across tiles.** The sheet is solved once per world on the planet grid (a pure function of the planet), then refined inside each region domain on cells of 4 km. The rim of that grid takes the planet-wide thickness wherever the planet has ice; elsewhere the rim is a wall. In the `Leave` stage, cells a finished neighbour owns (`Domain::finished`) are held at that neighbour's recorded `ice`/`iceAge`. Tiles are built in four parity passes, so of two neighbours one is always earlier: the later one continues the earlier one's sheet. Thickness on the domain's own 1 km cells is the coarse surface minus each cell's ground. Below Region everything is to be computed from the parent's records and world-anchored seeds through processes of finite reach (less than the 32-cell halo).

**Steady states, not a dated transient.** Stages are relaxed by continuation from warm to cold: the present from bare ground, the last maximum by cooling from it, colder ones by cooling on. A date is described by the share of a 100 kyr cycle its sawtooth spends nearest each stage (`stageShares`). Consequences to state honestly: no recessional still-stands; the present sheet is what bare ground grows, not the remnant of the last maximum (warming from the maximum instead leaves 38.8 % of the land under ice and almost no freed ground).

**Dating at Region.** Every fourth step (`Stride` in `Sheet.cpp`) the glaciations of those steps are worked into the ground at once, before that step's rivers; the last cycle of the window is left to `Leave`, which also books deposits by kind, routes meltwater and sinks the crust. `Present` copies the ice into `Domain::ice`/`iceAge` and returns the tile's record.

**Physics (constants in `constant/Ice.hpp`, each with its reason).**
- Flow: Glen n = 3, creep 5e-17 Pa⁻³/yr; sliding 5e-9 m/yr/Pa² on a thawed bed. Explicit pseudo-time relaxation, local step cell²/(15·D), at most 60 m of change a round, stop when the volume holds to 2e-4 over 40 rounds.
- Balance: `annualSnowBalance` at the ice surface; snowfall thinned by 6 %/°C the surface is colder than the ground the climate was solved for.
- Sea: a front loses 27·depth·thickness per metre of width and year, taken implicitly; only ground below the sea without load counts as sea floor (otherwise an island's ice cap sinks it, calves, rebounds and regrows for ever).
- Bed: Robin's steady profile with the rock's heat (`planet/fn/Heat.hpp`) plus shear heating (basal stress × creep speed). Without the shear term fast margins came out frozen and only the thick interior thawed. The thaw eases 20 % of the way to each reading, or thawed cells run out, thin, refreeze and never settle.
- Fine sliding: the ice a coarse cell would pass by sliding is shared among its 1 km cells by thickness cubed over the mean cube. **Do not go back to the local law (stress from fine thickness and coarse slope): a trough then speeds up as it deepens without limit and the bed runs away to tens of kilometres.** With the shared flux a trough carries no more than arrives and slows as it deepens.
- Wear: 1e-7 m per metre slid × rock factor (`sqrt(erodibility/granite)` × `quality` × (1 + 2.5·`weakness`)), loose cover first and 40 times faster. Cells are worked from the margin inwards so a floor that lowers as a whole is not held back. One working takes at most a quarter of the ice thickness.
- Overdeepening: a cell is not cut below `ground(n) − 1.5·(S − S(n))` of its easiest down-ice neighbour; next to open water it is not cut below `sea − sqrt(slide·thickness·0.917/27)`, the depth a front can stand in. By induction from the lip that bounds every basin.
- Debris: the last cycle's share is lodged with a reach of 15 km (at most 60 m a cell); the rest of a multi-cycle working goes to the margin. Bare ground keeps up to 60 m as end moraine (under the sea, up to the surface); the rest is booked as leaving with the meltwater. **Unbounded lodging or dumping builds towers of till that become nunataks and blow the next solve up.**
- Meltwater: follows `ground + 0.917·thickness`, steepest neighbour. A stream above 2e8 m³/yr keeps gravel and sand in its tunnel (esker fill); at a portal it takes `flow/(flow + 1e9)` of the moraine on and spreads gravel (6 km) and sand (22 km) down the bare ground; mud leaves.
- Crust: at `Leave` rock and layering are lowered by the present stage's sink plus what is still to come back of the last maximum's (e^(−13 kyr / 5 kyr)); `Tile::rebound` gets that rate.

**Wear rate is a calibration, not a measurement.** Measured rates are several times higher. This world replays 24–32 Myr of icehouse at Region, ten times Earth's Quaternary; with measured rates a lowland lost a kilometre of rock and sank under the sea although the planet says it is land. The rate is set so the window does about what Earth's 2.6 Myr did.

**The last sheet at the finer levels is one pass after settling, not a history.** Its parts act within a few cells (take-up per cell, digging against a box of one parent cell, lodging with a reach of 10 cells and a shelter search of 8, ridges within a tunnel's width), so a tile agrees with its neighbour wherever both see the same parent; where the ice comes out of a finished neighbour the neighbour's recorded `passing` load is fed in, else the load in balance with the bed. Drumlins are not seeded: till fills the cavity behind every rise of the bed, so their places come from the rock and relief the domain settled into, their direction from the parent's sliding and their size from the till supply. Eskers carry the parent's fill volume and the parent's chain of stretches, but their course is the domain's own: down the slope of the water head (bed + 0.917 · the ice over it, so the bed bends it at a twelfth of its relief) within 0.6 parent cells of the parent's route; where the head rises along the course by more than 2 m below what lies ahead, the water ponded and left no ridge; the stream always advances at least 0.3 of a step, so it passes through a pond. A tile's course starts on a finished neighbour's recorded ridge where that comes over the edge, and is drawn (50 m of head drawdown out to two corridors) into the ridge of a finished neighbour it runs into, or into the trunk it joins; at a confluence the stretch with more water carries on.

**At the finer levels the last sheet runs after the seam blend.** `blendSeams` assumes two tiles' versions of the same cells differ only in detail. The sheet's pass amplifies detail: it digs against a box mean, and the lodging is excitable. So the blend works on the ground before the sheet, and the sheet's change continues across the edge on its own. Region keeps its sheets before the blend because they are part of its settling (whole-domain relaxation).

**Basins follow the rock mass and the flow.** The pluck factor is rock kind × (1 + 2.5 · `weakness`) / `sound`. It is smoothed twice over a 2-cell box and compared with the mean over a parent cell. The ice digs a zone it runs along fully and one it crosses square-on at a quarter (`AcrossShare`), since it rides over a trench across its path. Depth approaches `BasinDepth` softly. A fault zone of the rock mass has a fixed width (`FaultWidth`) and straight planes, so its basin is straight; its depth varies with the parent's wear, the zone's weakness and the angle of the flow.

**A hollow in the ice surface keeps only what till stands in.** A cell with no neighbour lower on the parent's ice surface cannot pass its load on; it lodges up to its room (till repose against its lowest neighbour) and the meltwater takes the rest out of the domain, booked. Keeping all of it built columns of 1 km at Site, where the ice surface follows the Local ground at nunataks and margins.

**Water beyond the ice drains under it.** `washBeyond` floods the bare ground from the domain edge, the sea and every iced cell; a lake is ground the water stands over, its floor takes what arrives up to the spill level, deepest cells most, and the water spilling over its rim takes the rest out of the domain (booked). Treating the ice as a dam gave bare pockets enclosed by ice endless room; the price is that no proglacial lake forms against the ice, at Region or below. Over a flat the load runs on towards the cell the flood reached it from; the sea floor keeps what reaches it up to the sea's surface and passes the rest on down. What a hollow in the ice surface cannot hold also leaves the domain (booked), not handed on: there is no lower cell on the ice surface to hand it to.

**An esker is resolved at the level's cell, not at its own width.** A stretch keeps the volume its section can hold, spread over at least the cells its path crosses. So a 60 m-wide ridge at 64 m cells is drawn about two cells wide and half its real height; at Site (4 m) it would show its own width, if Site had tunnels.

**Finer levels are not dated.** Local settles for 6 Myr with twelve warm steps at the end, which fills or drains every lake and wears every fresh form. All sheet work at Local and Site therefore belongs in `Leave`, after settling. Forms wider than about 24 cells survive a child's settling because its averaged surface is held on the parent's (`TrendRadius` 12 in `Uplift.cpp`).

**Traps.**
- `stepDomain` also runs on hand-built domains with an empty `Planet`: `continentalIce` returns null and the hook does nothing. Keep it so.
- State of this stream hangs off one pointer in `Domain` and one in `Tile`; nothing is added to `allocate`/`allocateBare`.
- Names that clash inside `derivean::sim`: `Coarse`, `Settled`, `Free` are enumerators of the landscape. Do not name a constant after them.
- `levelHollows` in `Build.cpp` fills hollows of the parent shallower than 45 m (Local) or 6 m (Site) with rock before settling: a region-scale lake basin shallower than that is gone at Local unless `Leave` digs it again.
- `traceIce` in `Seed.cpp` walks up `rock + cover + iceAge` for at most 96 parent cells (6 km at Local). It follows sheet flow once `iceAge` holds the sheet but cannot find rock from farther away.
- A region tile of pass 3 needs up to 24 other tiles first. For a quick look pick a place whose region tile has even x and z index (`terra icesheet` prints the pass… once the tree builds again).
- xmake lists `src/**.cpp` when it configures: a new file is not compiled until `xmake config` runs again.
- `@simulation` is untracked in Git: no diff to recover a shared file from. Re-read a shared file immediately before an exact-match edit.

## 6. Contracts with other streams

Lines this stream added to shared files (all present now):

- `landscape/type/Terrain.hpp`: `struct SheetRecord;` and `struct SheetWork;` after `struct ShoreRecord;`; in `Tile`, `std::shared_ptr<const SheetRecord> sheet;` before `TileLedger ledger;`; in `Domain`, `std::shared_ptr<SheetWork> sheet;` after `VolcanicDomain volcanic;`.
- `landscape/fn/Process.hpp`, after `erupt`: a comment, `enum class IceStage : uint8_t { Step, Leave, Present };` and
  `std::shared_ptr<const SheetRecord> sheetIce(Domain&, FlowScratch&, const struct Setting&, IceStage, const std::function<const Tile*(TileKey)>* tiles = nullptr);`
- `landscape/fn/Replay.cpp`: `sheetIce(domain, scratch, setting, IceStage::Step);` before `cutAndFill` in `stepDomain`. In `replayTile`, `Leave` is called twice by level: `if (key.level == Level::Region) sheetIce(domain, scratch, setting, IceStage::Leave, &lookup);` right after the step loop (with a comment), and `if (key.level != Level::Region) sheetIce(domain, scratch, setting, IceStage::Leave, &lookup);` right after `settleSeams(domain, *tile);` (with a three-line comment). Changed 26 Nov from one call before `followNeighbours`. A Local or Site tile's `nowWide` therefore holds its ground before the last sheet. `tile->sheet = sheetIce(domain, scratch, setting, IceStage::Present, &lookup);` right before `extractTile`.
- `landscape/type/Terrain.hpp`: one comment line under the `nowWide` comment: `// At Local and Site it is the ground before the last ice sheet passed (landscape/fn/Replay.cpp).`
- `tool/main.cpp`: `if (command == "icesheet") return iceSheetCommand(arguments);`; `tool/Arguments.hpp`: `int iceSheetCommand(const Arguments& arguments);`.

`Tile::ice` and `Tile::iceAge` keep their meaning; at Region they now hold sheet ice (they were zero there).

Needed from other owners:

- **Valley glaciers (`Glacial.cpp`)**: its till split 0.5 / 0.2 / 0.3 is repeated in `constant/Ice.hpp` (`TillSplit`) because the original is private. One header for both would remove the copy.
- **Lake rule / `levelHollows`**: a basin cut in rock by ice must stay a lake at the child level (see traps).
- **Rivers under ice**: the region replay runs rivers on ground that lies under permanent ice in this world's climate (36 % of the land). Not handled here.
- **Far views**: the planet layer of `diagnostics/fn/Gather.cpp` and `chart` know no ice sheet; beyond the built tiles a view shows bare ground two kilometres below the ice surface of the tiles. `continentalIce(setting)` gives thickness per planet cell.
- **Climate**: with equal-area latitude bands and −22 °C at the poles, 36 % of the land carries ice today and the last maximum (only 2.2 °C colder in this world's greenhouse state) adds 3.3 %. If the owner wants more freed ground, that is a climate decision.
- **Sea level**: stages use the tile's sea level; the 11–28 m the extra ice of a maximum would draw down is not applied.

## 7. How to build, run and verify

```sh
S=/private/tmp/claude-501/-Users-marekhanzal-Project-marek-hanzal-derivean--claude-worktrees-derivean-landscape-engine-e64dc8/632cc3b3-3ddb-45d4-9dc7-715e32a5db02/scratchpad
```

That scratch directory is session-local and may be gone. What it held, so it can be rebuilt in minutes:

- `build.sh`: `cd @simulation; export XMAKE_CONFIGDIR=$PWD/../.out/simulation/xmake-ice; xmake config --yes --mode=release --builddir=../.out/simulation/build-ice; xmake build --yes terra simulation-tests`, printing errors and exiting non-zero on failure. A private build directory keeps clear of other agents' builds in `.out/simulation/build`.
- `mk.sh <name>`: compiles one scratch `.cpp` together with `src/icesheet/fn/*.cpp` and `service/*.cpp` against the last good `libderivean-simulation.a`; works while the shared tree is broken.
- `plateau.cpp`, `conv.cpp`, `fjord.cpp`, `shield.cpp` + `rig.hpp`: the hand-built rigs now in `test/IceSheet.cpp`, with profiles and pictures.
- `probe.cpp x z out`: builds the region tile there with `ICE_OFF` and without (the switch is gone from `Sheet.cpp`; put `if (std::getenv("ICE_OFF")) return nullptr;` back locally at the top of `sheetIce` for an A/B run), prints heights, wear, erosion and uplift by class (under ice now / freed / never) and writes `probe-off/on/diff.png`.
- `spots.cpp x z`: in the region tile there, freed cells with the most till, esker fill, wear and moraine, as `--x --z` arguments.
- `seamprobe.cpp x z`: 2 × 2 Local mosaic with and without the finer pass; row means across the horizontal seam of (on − off) ground, till, worn and recorded `passing`, plus each tile's pass.
- `eskerstat.cpp x z [tiles] [level]`: in a square of finer tiles, the tallest esker cell, cells over 1 m, connected runs (and single cells) and mean width across a run.
- `count2.cpp`: like `count.cpp`, and for the focus Local and Site tiles the cell where loose cover stands most above its repose (the measure of `testReliefCover`), with its sheet record.
- `localprobe` with `ONLY_ON=1` skips the run without the pass.
- **Private mirror** (`mb.sh [targets]`): rsyncs the worktree's `@simulation` to `mirror/sim` (without `.xmake`, `build`, `mise.toml`), touches every file it replaced, patches the lab switch `ICE_FINER_OFF` into the mirror's `sheetFiner`, and builds into `mirror/build` with xmake 3.1.2 by absolute path (`XMAKE_CONFIGDIR=mirror/xmakecfg`). Other streams' half-saved files only break it when they are in the copy; the shared tree is never built. Test drivers: `localmain.cpp` + `test/IceSheetLocal.cpp`, `testmain.cpp` + `test/IceSheet.cpp`, linked against `mirror/build/macosx/arm64/release/libderivean-simulation.a` with `-Imirror/sim/src -Imirror/sim/test`. Without the touch, a file restored with an older timestamp than its object was not recompiled.
- `column.cpp x z [level]`: one finer tile with and without the pass; the cells standing more than 20 m above all eight neighbours, with cover, ice and the sheet record there.
- `landforms.cpp x z [tiles] [level]`: counts on freed ground against F1/F3: lake share, lakes by size, shoreline index, rock or drift; relief per km and per 10 km; till hills and rock knobs (rise over a 640 m mean, length along the flow, drumlin-shaped); esker runs; hollows in drift holding water.
- `seamprobe2.cpp x z`: all four seams of a 2 × 2 Local mosaic, (on − off) ground and till by row across each, and which way the ice slid across it.
- `count.cpp`: tiles finished after `surveyAt` at the test world's focus, at each level, with and without the finer pass. Result 26 Nov: 3 tiles (one Region, one Local, one Site) either way. The finer pass reads only tiles in `dependencies(key)` through the atlas lookup, so it cannot start builds.
- `localprobe.cpp x z out [tiles] [exaggeration] [level]`: a square of finer tiles with `ICE_FINER_OFF` and without (same local patch, at the top of `sheetFiner`; about 3 min per pass for 2 × 2 Local); lakes, slope, till; `local-off/on/left/diff.png`.
- Compile any of them: `c++ -std=c++20 -O2 -I@simulation/src <file> .out/simulation/build-ice/macosx/arm64/release/libderivean-simulation.a -o <name>` (rebuild the library first when a header changed).

Real world, from the worktree root:

```sh
T=.out/simulation/build-ice/macosx/arm64/release/terra
$T icesheet --out .out/icesheet                       # stages, planet map, places (about 30 s for the planet)
$T icesheet --x 592981 --z 7584093 --out .out/icesheet   # census and maps of the region tile there
$T map --level 1 --tiles 2 --x … --z … --layer ice --out .out/icesheet
```

The shared tasks are `mise -C @simulation run build` and `mise -C @simulation run test`; run both before reporting. Pictures go to `.out/icesheet/`; read them with the Read tool beside the catalogue's photographs.

## 8. Open questions

- **For the level-consistency stream: Site seams and a repeated patch, without any ice.** Seed 42, age 4.6e9, Site 2 × 2 mosaic whose first tile holds `--x 7042816 --z 13263360` (`gatherMosaic(world, Level::Site, {7042816, 13263360}, 2)`). The finer ice pass was switched off for this run (the `ICE_FINER_OFF` patch, section 7), so this is the plain settled replay. Picture: `.out/icesheet/site2/local-off.png` (relief ×3, water blue). What it shows:
  - straight lines along both tile edges (x = 256 and z = 256 of the 512² picture), a dark groove down the vertical one;
  - in all four tiles a rough, pitted patch of about the same outline at about the same place in the tile, with smooth ground around it.
  Rebuild with `localprobe 7042816 13263360 <out> 2 3 3` from section 7. The run was on tree 08f5a7e, before 38b4939 (window-start strata column); not re-checked since.

- A freed lowland 60 m above the sea is half drowned (crust still rebounding plus 20 m net wear): Earth-like (Hudson Bay, Baltic), but the owner may want to see it.
- Does the 4 km relaxation agree across a tile edge well enough that the end moraine does not step at a seam?
- Drumlins come out as till tails behind rises and as slowly decaying till waves; whether fields of separate egg-shaped hills appear needs Site pictures and a count.
- Tile-time cost of a glaciated region tile against the 7–9 s of one without ice: not yet measured on land.
