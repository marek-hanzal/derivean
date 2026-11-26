# Hand-over: ice sheets (`@simulation/src/icesheet`)

Describes the present state. Rewrite it when the state changes; history belongs in Git.

## 1. Goal

The owner's verdict: the world looks the same everywhere. A third of Earth's land was under ice at the last glacial maximum and still shows it. The simulation has valley glaciers at Local and Site (`landscape/fn/Glacial.cpp`) and nothing at continental scale.

This stream owns a new domain `@simulation/src/icesheet/**` (+ `test/IceSheet*.cpp`, `tool/IceSheet*.cpp`) that must produce, from simulated processes and never as props or painted noise:

1. **The sheet**: accumulates where the model's climate gives a positive balance, flows under its own surface slope (shallow ice), ends where melt or the sea takes it; extent, thickness, flow direction, thawed or frozen bed; crust sinks under it and rebounds.
2. **Rock**: little erosion on frozen uplands, troughs along fast outlets, overdeepened below sea level and drowned as fjords with sills; on shields rock basins along joints and fracture zones that become lakes the water routing must find.
3. **Deposits**: till sheets, drumlin fields, end moraines, kettled hummocky ground, eskers along subglacial water routes, outwash plains.
4. **Levels**: Region carries the sheet and the big forms; Local the drumlins, eskers, kettles and lake basins; Site their slopes and material.

Catalogue targets are in `docs/reference-landscapes.md` (A2, A3, F1–F4, G1). Required tests and the `terra icesheet` statistics are listed in section 4.

## 2. State now

**No code of this stream exists yet.** Reading and design are done; the first file to write is the solver (section 4, step 1).

Measured on the shared machine (16 cores, heavily loaded):

- Planet evolution: 31 s per `terra` run (not cached between runs). One Region tile of open ocean: 8 s.
- A full build into a private build directory: 33 s (section 7).

Facts about the world that shape the design (seed 42, age 4.6e9, verified with a scratch program against `climate/**`):

- `greenhouseOffset` is +1.79 °C now, so the glacial amplitude is only 2.70 °C and the last maximum is 2.13 °C colder than the present (the sawtooth peaks at phase 0.87, i.e. about 13 kyr before the present). Between 8 and 32 Myr ago the maxima were up to 6.3 °C colder than the present.
- Climate bands are a triangle wave in Z, so every degree of latitude has the same area: land poleward of 45° is half the planet. With a 14 °C seasonal half-range the snow balance at sea level is positive today poleward of about 63° and at the last maximum poleward of about 58°.
- Consequence: this world has **ice sheets today** over polar land, a band a few hundred kilometres wide that was freed since the last maximum, and a much wider belt (to about 50°) that only older, deeper glaciations reached. All three must come out: `Tile::ice` (now), `Tile::iceAge` (last maximum), and cumulative bedrock work from the older ones.
- A region step is 400 kyr, four glacial cycles; step times are always at phase 0, so `readClimate` never sees a glacial climate at Region level.

## 3. In progress

Nothing half-written. Next action is step 1 of section 4.

## 4. Next steps

1. `icesheet/constant/Ice.hpp`, `icesheet/type/IceGrid.hpp`, `icesheet/fn/Balance.hpp`, `icesheet/fn/Flow.{hpp,cpp}`: the shallow-ice relaxation on any square grid (used at 65 km on the planet and at 4 km inside a region domain). Test: a cold plateau grows a sheet steep at the margin and flat inside; a warm one grows none.
2. `icesheet/fn/Continental.cpp` + `icesheet/service/Glaciations.cpp`: steady sheets on the planet grid for a ladder of coolings (present, last maximum, deeper maxima of the replayed window), memoised per (seed, age, planet time). `terra icesheet` prints extent, volume, share of land glaciated and writes a planet map.
3. Region: sheet on the domain (4 km solve, 1 km thickness as surface minus bed), `Present` stage writes `ice` and `iceAge`.
4. Region wear and debris (`Step` and `Leave` stages): thaw, sliding, wear, overdeepening limit, flotation; till with a transport length, end moraine, subglacial water routes, eskers, outwash. Hand-built domains: coastal range with a valley, tilted shield with fracture zones, lowland margin.
5. Local `Leave`: differential scour along the rock structure, cover rebuilt from the parent's records as drumlins, esker ridges, hummocky moraine with kettles, outwash.
6. Site `Leave`: stoss-and-lee knobs, small kettles.
7. Tests (`test/IceSheet*.cpp`, registered in `test/Main.cpp`): plateau and warm plateau; outlet through a range cuts a trough deeper than the valley beside it and overdeepens it; jointed shield keeps closed rock basins as lakes after routing; still-stand leaves a moraine and outwash beyond it; drumlins align with the flow and are not evenly spaced; two neighbouring tiles agree on the shared edge; one thread equals the pool bit for bit; ledger closes.
8. `terra icesheet --x --z`: extent and thickness at the maximum, trough depths, lake cover, counts and sizes of drumlins, eskers and kettles, share of land glaciated.
9. Real world at places found by search (cold land at the maximum), pictures with `terra view` and `terra map --layer ice` against the catalogue.
10. Rows for `docs/simulation.md` (evolved / closed form / seeded / absent), independent reviewer, report.

## 5. Decisions and why

**One sheet across tiles.** The sheet is solved once per world on the planet grid (a pure function of the planet), then refined inside each region domain. A domain's rim takes the planet-scale thickness; cells a finished neighbour owns (`Domain::finished`, hold `Settled`) take that neighbour's recorded ice for the two recorded states (now, last maximum). Tiles are built in four parity passes, so of two neighbours one is always earlier: the later one continues the earlier one's sheet, which is what makes the shared edge agree. Everything below Region level is computed from the parent's records and world-anchored seeds through processes of finite reach (less than the 32-cell halo), never from the tile's own outline.

**Steady states, not a dated transient.** Sheets are relaxed to the steady state of each cooling of a ladder, by continuation: last maximum from bare ground, the present by warming from it (so present ice is the remnant of the glacial sheet), deeper maxima by cooling further. A dated 100 kyr transient on the planet grid costs 5–10 s per cycle single-threaded and the Region needs sheets for maxima the last cycle never reached. Label it "evolved to the steady state of each climate"; recessional still-stands are absent.

**Why not only a closed form at Region.** Reading the planet-scale surface and subtracting the region's bed is exactly tile-consistent and cheap, but its margin is only as good as 65 km cells and its profile is a ramp. A 4 km relaxation inside the domain gives the margin, lobes and icefields on high ground; thickness at 1 km is that surface minus the 1 km bed. This is the fallback if the 4 km solve proves unstable at tile edges.

**Physics chosen (all constants go to `icesheet/constant/Ice.hpp`).**
- Flow: Glen n = 3, creep 5e-17 Pa⁻³/yr; sliding 5e-9 m/yr/Pa² on a thawed bed. Explicit pseudo-time relaxation with a local step of cell²/(15·D).
- Balance: `annualSnowBalance` at the ice surface (lapse rate), snowfall thinned by 6 %/°C of cooling at the surface so a dome starves instead of running away.
- Sea: a front in water loses 27·depth·thickness per metre of width and year. A front then holds only in water shallower than √(flux/29) ≈ 60–180 m, which is where fjord sills should come out.
- Bed: thawed where the surface temperature plus the geothermal warming through the ice (Robin's closed form, heat from `planet/fn/Heat.hpp`) reaches the pressure melting point. Thin ice on high cold ground stays frozen and protects it.
- Wear: K·sliding speed·rock factor, loose cover first and about 15 times faster; rock factor from `traits.erodibility`, `Domain::quality` and `Domain::weakness`, the same structure rivers follow.
- Overdeepening: a cell may not be cut below `bed(n) − 1.5·(S − S(n))` of its easiest down-ice neighbour (adverse bed slope at most 1.5 times the surface slope, the supercooling limit). This is a local rule; applied step after step it bounds shield basins at metres to tens of metres and lets a fjord deepen inland from its sill by about 15 m per km.
- Debris: travels a transport length of 10–20 km before it is lodged as till; what is still in the ice at the terminus is the end moraine; the meltwater share goes down the hydraulic potential (0.917·surface + 0.083·bed) to eskers and outwash.

**Dating at Region.** Each step holds four cycles. A cell's wear per cycle is the sum over the ladder of (time the sawtooth spends near that cooling) × (wear rate under that sheet). The fluvial step that follows reworks the debris. After the last step the `Leave` stage redoes the last cycle alone, so its deposits are fresh and its basins are empty of sediment: that is what makes lakes exist now.

**Finer levels are not dated.** Local settles for 6 Myr with twelve warm steps at the end, which fills or drains every lake and wears every fresh form. So at Local and Site all sheet work happens in `Leave`, after settling: rock is scoured differentially (zero mean: the parent already holds the average) and the loose cover under the last sheet is rebuilt from the parent's records. Forms wider than about 24 cells survive a child's settling because the child's averaged surface is held on the parent's (`TrendRadius` 12 in `Uplift.cpp`).

**Traps.**
- `stepDomain` is also called on hand-built domains with an empty `Planet` (`measure/fn/PlaneDomain.cpp`): the hook must return when `planet->elevation` is empty.
- `allocateBare` (measure) and `allocate` (Build.cpp) list every per-cell array of `Domain`. This stream allocates its own state lazily behind one pointer instead of adding arrays there.
- `levelHollows` in `Build.cpp` fills hollows of the parent shallower than 45 m (Local) or 6 m (Site) with rock before settling. A region-scale lake basin shallower than that is gone at Local unless `Leave` digs it again.
- `traceIce` in `Seed.cpp` walks up the surface `rock + cover + iceAge` for at most 96 parent cells (6 km at Local). It follows sheet flow correctly once `iceAge` holds the sheet, but cannot find rock from farther away.
- `@simulation` is untracked in Git: there is no diff to recover a shared file from. Other streams edit `Replay.cpp`, `Process.hpp`, `Stability.cpp`, `Talus.cpp` and the lake rule concurrently; re-read a shared file immediately before an exact-match edit.
- Other streams build in `.out/simulation/build`. Use the private build directory of section 7 during development.

## 6. Contracts with other streams

Planned additions to shared files; **none is made yet**. Update this list with the exact lines once they are.

- `landscape/fn/Process.hpp`: one declaration,
  `enum class IceStage : uint8_t { Step, Leave, Present };`
  `void sheetIce(Domain&, FlowScratch&, const struct Setting&, IceStage, const std::function<const Tile*(TileKey)>* tiles = nullptr);`
- `landscape/fn/Replay.cpp`: one call per place — `Step` in `stepDomain` before `cutAndFill`; `Leave` in `replayTile` right after the step loop (before `followNeighbours`/`settleSeams`, so seams are blended with the fresh deposits in place); `Present` right before `extractTile` (after the final `glaciate`, which resets `Domain::ice`).
- `landscape/type/Terrain.hpp`: one pointer in `Domain` (working state) and one in `Tile` (the record children and tools read), both to types of this domain, forward-declared.
- `landscape/fn/Extract.cpp`: one line handing the record to the tile.
- `Tile::ice` and `Tile::iceAge` keep their meaning and now also hold sheet ice at every level.

Needed from other owners (to be confirmed once the code runs):

- **Valley glaciers (`Glacial.cpp`)**: its constants (`Plucking`, `MoraineLimit`, the till split 0.5 / 0.2 / 0.3) are private. For sheet and valley ice to share one law they should move to a header both include.
- **Lake rule**: a basin cut in rock by ice must stay a lake; `levelHollows` must not fill it at the child level (see traps).
- **Rivers under ice**: the region replay runs rivers on ground that lies under permanent ice in this world's climate. Not handled by this stream.
- **Sea level**: the sheets use a sea lowered by their own volume to decide where a front grounds; the tile's sea level is not changed.

## 7. How to build, run and verify

All from the worktree root. A private build directory keeps this stream clear of other agents' builds:

```sh
cd @simulation
export XMAKE_CONFIGDIR="$PWD/../.out/simulation/xmake-ice"
xmake config --yes --mode=release --builddir=../.out/simulation/build-ice
xmake build --yes terra simulation-tests
../.out/simulation/build-ice/macosx/arm64/release/simulation-tests
../.out/simulation/build-ice/macosx/arm64/release/terra map --level 1 --tiles 1 --x 0 --z 0 --layer ice --out ../.out/icesheet
```

The shared tasks are `mise -C @simulation run build` and `mise -C @simulation run test`; run them once before reporting. Pictures and logs of this stream go to `.out/icesheet/`. Read pictures with the Read tool and compare with the photographs linked in the catalogue.

## 8. Open questions

- Whether the 4 km relaxation agrees well enough across tile edges for the still-stand moraine not to step at a seam; the fallback is the closed form of section 5.
- Wear constant K: to be set so that an outlet reaches several hundred metres to more than a kilometre over the icehouse history of a place and a shield a few metres per glaciation. Not yet measured.
- Whether drumlins can come from an evolved bed process (till carried down-ice around stiff patches, sheltered in their lee, at most 28 rounds so the reach stays inside the halo) or need the closed form (seeded nuclei, size and elongation from sliding speed and till supply). To be prototyped.
- The share of land under ice at the last maximum is expected well above Earth's 32 % because of the equal-area latitude bands; not yet measured.
- Tile-time cost of the region work: estimated at a few seconds per glaciated region tile; not yet measured.
