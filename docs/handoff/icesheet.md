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

**Works, with tests in the suite (`test/IceSheet.cpp`, `test/IceSheetLocal.cpp`, registered in `test/Main.cpp` before `testLandscapePlane`; both pass, 6 s and 0.2 s, run through scratch drivers because the suite currently stops earlier in `testAeolian`, another stream's):**

- `fn/Flow.cpp` — shallow-ice relaxation on any square grid. Test: a cold plateau grows a dome kilometres thick, flat inside and steep at its margin; a warm one stays bare; pool of 4 = no pool, bit for bit.
- `fn/Continental.cpp`, `service/Glaciations.cpp`, `fn/Dating.cpp` — planet-wide stages for a world, memoised. Seed 42, age 4.6e9: five stages (present, last maximum at −2.16 °C, three colder down to −5.63 °C). Land under ice: **36.2 % now, 39.5 % at the last maximum, 44.9 % in the coldest stage** (Earth: 10 % and 32 %). 1.3–8 s once per world.
- Region pipeline (`fn/Region.cpp`, `Wear.cpp`, `Debris.cpp`, `Meltwater.cpp`, `Work.cpp`, `Sheet.cpp`), hook live in `Replay.cpp`. Tests on hand-built domains: a valley through a coastal range is cut 150 m below the sea with a sill at its mouth while the ridge beside it keeps 1 100 m and the frozen plateau is not worn; on a shield with fracture zones the zones become lakes in rock (9 % of the ground the sheet left, lake cells three times as weak as the mean), with end moraine, outwash and an esker; the ledger closes to 1e-8; pool = no pool.
- Region on real land (seed 42). Inland freed lowland at `--x 7075912 --z 13325105`: rock worn 128 m on freed ground (mean), the ground 51 m lower than without ice, of which about 30 m is crust still to come back (e^(−13/5) of the last maximum's sink): half the freed lowland, 60 m above the sea without ice, is now under the sea, as Hudson Bay or the Baltic. Coastal range at `--x 11585794 --z 4285552`: the coast is more indented, ground under present ice 340 m lower (sink and wear). Pictures `.out/icesheet/probe2/` and `probe-coast/` (`probe-diff.png`: red lowered, hatched = under the last maximum).
- **The last sheet at Local and Site** (`fn/Parent.cpp`, `Local.cpp`, `Lodge.cpp`, `Esker.cpp`, types in `type/Parent.hpp`). After a finer domain has settled, `Leave` reads the parent's record and ice through the `tiles` lookup; the cover is taken up (as much as one glaciation strips), weak rock is dug against the mean of a parent cell (up to 50 m at Local, 10 m at Site), till is carried down the parent's ice surface and lodged, filling the cavity behind every rise to a line falling at 5 %, so it piles in tails behind knobs; tunnel fills are set aside from the cover before and stand as ridges along the parent's meltwater path after; beyond the ice the load stays as moraine no steeper than till stands (24°). `Present` puts the parent's ice surface over the domain's own ground (valleys get more). Test on a hand-built plain: basin dug in a fracture zone, till twice as thick in the lee of each knob as on its stoss side and drawn out along the flow, a ridge along the stream, ledger closes, a tile whose up-ice halo a finished neighbour owns gets the same till within 10 %, ground the sheet never reached gets no record. Real Local tiles (2 × 2 at `--x 7052800 --z 13263360`, a freed till lowland, `.out/icesheet/local4/`): lakes 0.1 % → 8.3 % of the land, mean slope 1.4° → 2.3°, till 6.5 m mean, rock worn 134 m (parent); till tails and rock basins aligned with the flow (`zoom-on.png`, `local-left.png`: brown = till). Judged against F1/F3 it is not there yet, see "Broken".
- `tool/IceSheetCommand.cpp` — `terra icesheet` (stages, planet map, freed places; with `--x --z [--level] [--tiles]` a census and two maps).

**Not yet seen working:**

- Site tiles with the last sheet: the code is the same at Site (Local parent) but no Site tile has been built and looked at.
- Eskers at Local in the real world: the inland tile has no tunnel fill on freed land; the coastal tile has (`--x 11582976 --z 4307456`, 4.3 m of fill), not yet built at Local.
- Kettles and hummocky dead-ice ground: not modelled.
- The full suite: it stops in `testAeolian` (another stream) before reaching these tests.

**Broken or known wrong:**

- **Tile seam at Local** (`local4/seam-diff.png`): across the horizontal edge between the two left tiles the ground the sheet raised steps by a few metres (more till in the lower tile). The recorded `passing` feed matches on the hand-built rig; in the real world something else differs across the edge (load guessed for a halo with no finished neighbour, the repose room against the neighbour's tilled ground, or the seam blend of `Extract.cpp` that runs after `Leave`). First thing to find.
- **Look at Local, against F1/F3**: many lakes are one or two 64 m cells (speckle), and rock is dug in short dashes along the grid where the rock-structure field has its joints; rivers that follow fracture zones are dug 20 m deeper with a till bank on their lee side, so they read as straight leveed channels. The ice edge from the parent's 1 km cells is a straight-sided block (top right of `local-diff.png`). Real shield country has lakes of every size up to kilometres with rough rims, and till in separate streamlined hills.
- Reviewer findings not yet done (26 Nov): esker cover take-up and ridge height still scale with sums over the whole domain (a seam risk where cover runs short; take per stretch instead); moraine beyond the ice is held to one cell's repose and the surplus is booked as `sedimentOut` at an interior cell (should spread outward or go to outwash); `passing` is a total, so the grain mix steps at a seam; digging only lowers and its places follow the rock `quality` field where there is no weakness (a seeded field: near the "no noise shapes relief" rule); at Site `iceThen` is the Local tile's `iceAge`, which includes Local's own valley glaciers.
- The lodging is excitable: wherever load enters a domain without its history (the rim, a halo with no finished neighbour) it sets off a train of till waves about 20 cells long that decays slowly. Inside the tile it is driven by the halo's 32 cells; with a finished neighbour the recorded `passing` load is taken up instead. Watch the 2 × 2 pictures for seams.
- `Continental.cpp` reports a thickest cell of 8 131 m: a deep sea-floor cell enclosed by iced cells takes the coarse surface over its own bed when the half-resolution result is handed to the full grid. Harmless for extents, wrong as a number.
- The shared build breaks whenever another stream's file is half-written; a silent failure leaves an old binary. Use the build script of section 7, which fails loudly.

## 3. In progress

Nothing half-done. Third run (Opus, 26 Nov) ended after: first review's findings fixed; second review done (findings listed under "Broken"; two small ones fixed: negative cover no longer books erosion, a cell whose flow points at no lower surface passes its load down the surface instead of keeping it all); the temporary `ICE_OFF` / `ICE_FINER_OFF` switches removed, so A/B probes now need a local patch (one early `return nullptr` in `sheetIce` / `sheetFiner`).

## 4. Next steps

0. Find and remove the Local tile seam (section 2, "Broken"); then the speckle lakes and grid dashes (dig against a wider plucking footprint, or let small hollows fill), then the reviewer's list.
1. Look at Site tiles under the last sheet (flutes, knobs) and at an esker at Local on the coastal tile; tune `ShelterSlope`, `LodgeCells`, `BasinDepth` against F1/F3 (lakes 10–30 % on a shield, drumlins 250–1 000 m long, 5–50 m high, 300–1 000 m apart).
2. Kettles and hummocky ground where the last sheet stagnated (moraine and outwash under thin, slow ice): buried ice melting out of the cover.
3. `terra icesheet`: counts and sizes of drumlins, eskers and lakes at Local.
4. A real 2 × 2 Local mosaic across a tile edge under ice: no step in till or eskers.
5. Pictures with `terra view` against the catalogue (F1, F2, F3, G1); rows for `docs/simulation.md`; reviewer; report.
6. Region: stage-by-stage still-stands (recessional moraines) are still absent; the inland freed lowland is half drowned by the crust still to rebound, which is Earth-like but worth the owner's eye.

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

**The last sheet at the finer levels is one pass after settling, not a history.** Its parts act within a few cells (take-up per cell, digging against a box of one parent cell, lodging with a reach of 10 cells and a shelter search of 8, ridges within a tunnel's width), so a tile agrees with its neighbour wherever both see the same parent; where the ice comes out of a finished neighbour the neighbour's recorded `passing` load is fed in, else the load in balance with the bed. Drumlins are not seeded: till fills the cavity behind every rise of the bed, so their places come from the rock and relief the domain settled into, their direction from the parent's sliding and their size from the till supply. Eskers follow the parent's meltwater path (smoothed through the parent's cell centres; the path is a Region result, the parabolic section a tunnel fill's) and carry the parent's fill volume.

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
- `landscape/fn/Replay.cpp`: `sheetIce(domain, scratch, setting, IceStage::Step);` before `cutAndFill` in `stepDomain`; `sheetIce(domain, scratch, setting, IceStage::Leave, &lookup);` right after the step loop in `replayTile`; `tile->sheet = sheetIce(domain, scratch, setting, IceStage::Present, &lookup);` right before `extractTile`.
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

- A freed lowland 60 m above the sea is half drowned (crust still rebounding plus 20 m net wear): Earth-like (Hudson Bay, Baltic), but the owner may want to see it.
- Does the 4 km relaxation agree across a tile edge well enough that the end moraine does not step at a seam?
- Drumlins come out as till tails behind rises and as slowly decaying till waves; whether fields of separate egg-shaped hills appear needs Site pictures and a count.
- Tile-time cost of a glaciated region tile against the 7–9 s of one without ice: not yet measured on land.
