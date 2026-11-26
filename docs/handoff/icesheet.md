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

**Works (checked by the scratch programs named in section 7, not yet by tests in the suite):**

- `fn/Flow.cpp` — shallow-ice relaxation on any square grid. A cold plateau of 600 m grows a dome 3 km thick, steep at the margin, flat inside, ending in the sea as a thin floating fringe. Same rounds and same cells with a pool of 4 threads and with none.
- `fn/Continental.cpp`, `service/Glaciations.cpp`, `fn/Dating.cpp` — planet-wide stages for a world, memoised. Seed 42, age 4.6e9: five stages (present, last maximum at −2.16 °C, three colder down to −5.63 °C). Land under ice: **36.2 % now, 39.5 % at the last maximum, 44.9 % in the coldest stage** (Earth: 10 % and 32 %). Cost 1.3–8 s once per world, depending on how busy the pool is.
- `fn/Region.cpp`, `fn/Wear.cpp`, `fn/Debris.cpp`, `fn/Meltwater.cpp`, `fn/Work.cpp`, `fn/Sheet.cpp` — the Region pipeline end to end; the hook is live in `Replay.cpp`. On a hand-built coastal range the ledger closes to 1e-8 of the volume moved, the valley through the range is cut far deeper than the ground beside it, the plateau stays frozen to its bed while the valley thaws, and the profile rises to a sill at the mouth.
- `tool/IceSheetCommand.cpp` — `terra icesheet` prints the stages, writes a planet map and names places freed since the last maximum; with `--x --z` it builds the region tile and prints a census and two maps.

**Not yet seen working:**

- A real region tile on freed **land**. The two tried were sea (one because the first wear rate, 4e-6, ground a kilometre off and sank the lowland; the rate is now 7e-7 and the rebound feed is fixed, but no land tile has been rebuilt since).
- Nothing below Region level: `sheetIce` returns at once for Local, Site and Ground.
- No test of this stream is in the suite. `mise run test` has not been run with the hook live.

**Broken or known wrong:**

- Over many workings with no rebound and no rivers (the scratch rig), a thawed plateau is ground below sea level and its ice floats off. Whether the real replay (rebound 85 %, rivers, the lower rate) keeps land is the next thing to look at.
- `Continental.cpp` reports a thickest cell of 8 131 m: a deep sea-floor cell enclosed by iced cells takes the coarse surface over its own bed when the half-resolution result is handed to the full grid. Harmless for extents, wrong as a number.
- The shared build breaks whenever another stream's file is half-written; a silent failure leaves an old binary. Use the build script of section 7, which fails loudly.

## 3. In progress

Waiting for the shared tree to compile again (groundwater was mid-edit), then: `terra icesheet --x … --z …` on an inland freed place whose region tile is of pass 0 (so one tile is built, not seventeen), to see land, ice, lakes and deposits with the new wear rate. No file of this stream is half-edited.

## 4. Next steps

1. Region on real land: census and maps at two or three freed places (inland lowland; coast with relief). Tune `WearRate`, `LooseGain`, `Overdeepening` against trough depth and lake cover. Check the sheet across a tile edge on a 2 × 2 map.
2. Hand-built region rigs for the other two cases: tilted shield with fracture zones (closed rock basins, kept as lakes by `floodLevels`/`routeWater`), lowland margin (moraine ridge, outwash beyond).
3. Local `Leave` (in `fn/Sheet.cpp`, new files `fn/Local*.cpp`): read the parent's `SheetRecord`, `ice`, `iceAge` through the `tiles` lookup; differential scour along `Domain::weakness`/`quality` scaled by the parent's `worn`; loose cover under the last sheet rebuilt from the parent's `till`, `moraine`, `outwash`, `esker` as drumlins, a ridge along the esker course, hummocks with kettles. `Present`: `ice`/`iceAge` = parent's ice surface minus local ground, max with valley ice.
4. Site `Leave`/`Present`: same reading of the Local parent; stoss-and-lee knobs, small kettles.
5. Tests `test/IceSheet*.cpp` registered in `test/Main.cpp` (list in section 1 of the brief: plateau/warm plateau; trough deeper than the valley beside it and overdeepened; shield basins kept as lakes; moraine and outwash at a still-stand; drumlins aligned and not evenly spaced; two tiles agree on the shared edge; one thread equals the pool; ledger closes).
6. `terra icesheet`: add counts and sizes of drumlins, eskers and kettles at Local.
7. Pictures (`terra view`, maps) against the catalogue; rows for `docs/simulation.md`; independent reviewer; report.

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
- Wear: 7e-7 m per metre slid × rock factor (`sqrt(erodibility/granite)` × `quality` × (1 + 2.5·`weakness`)), loose cover first and 40 times faster. Cells are worked from the margin inwards so a floor that lowers as a whole is not held back. One working takes at most a quarter of the ice thickness.
- Overdeepening: a cell is not cut below `ground(n) − 1.5·(S − S(n))` of its easiest down-ice neighbour; next to open water it is not cut below `sea − sqrt(slide·thickness·0.917/27)`, the depth a front can stand in. By induction from the lip that bounds every basin.
- Debris: the last cycle's share is lodged with a reach of 15 km (at most 60 m a cell); the rest of a multi-cycle working goes to the margin. Bare ground keeps up to 60 m as end moraine (under the sea, up to the surface); the rest is booked as leaving with the meltwater. **Unbounded lodging or dumping builds towers of till that become nunataks and blow the next solve up.**
- Meltwater: follows `ground + 0.917·thickness`, steepest neighbour. A stream above 2e8 m³/yr keeps gravel and sand in its tunnel (esker fill); at a portal it takes `flow/(flow + 1e9)` of the moraine on and spreads gravel (6 km) and sand (22 km) down the bare ground; mud leaves.
- Crust: at `Leave` rock and layering are lowered by the present stage's sink plus what is still to come back of the last maximum's (e^(−13 kyr / 5 kyr)); `Tile::rebound` gets that rate.

**Wear rate is a calibration, not a measurement.** Measured rates are several times higher. This world replays 24–32 Myr of icehouse at Region, ten times Earth's Quaternary; with measured rates a lowland lost a kilometre of rock and sank under the sea although the planet says it is land. The rate is set so the window does about what Earth's 2.6 Myr did.

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
- `plateau.cpp` (dome on a plateau, prints the profile), `conv.cpp` (rounds and time per stage on a cached copy of the planet's fields), `fjord.cpp` + `rig.hpp` (hand-built region domain: sea, coastal range with two valleys, plateau; `workSheets` fifteen times; prints the valley and ridge profiles and the ledger). These are the seeds of `test/IceSheet*.cpp`.

Real world, from the worktree root:

```sh
T=.out/simulation/build-ice/macosx/arm64/release/terra
$T icesheet --out .out/icesheet                       # stages, planet map, places (about 30 s for the planet)
$T icesheet --x 592981 --z 7584093 --out .out/icesheet   # census and maps of the region tile there
$T map --level 1 --tiles 2 --x … --z … --layer ice --out .out/icesheet
```

The shared tasks are `mise -C @simulation run build` and `mise -C @simulation run test`; run both before reporting. Pictures go to `.out/icesheet/`; read them with the Read tool beside the catalogue's photographs.

## 8. Open questions

- Does land stay land under 24 Myr of sheets with the rate of 7e-7 and the plate's rebound? Not yet rebuilt.
- Does the 4 km relaxation agree across a tile edge well enough that the end moraine does not step at a seam?
- Drumlins: an evolved bed process (till carried down-ice around stiff patches and sheltered in their lee, at most 28 rounds so its reach stays inside the halo) or the closed form (seeded nuclei; size, elongation and height from sliding speed and till supply). To be prototyped at Local.
- Tile-time cost of a glaciated region tile against the 7–9 s of one without ice: not yet measured on land.
