# Hand-over: wind and drylands (`@simulation/src/aeolian`)

Describes the present. Rewrite as things change; history belongs in Git.

## 1. Goal

The owner's verdict: the world looks the same everywhere, and nothing lets wind move anything. This stream owns what wind does to loose ground and the dry-basin forms that go with it. Everything must arise from simulated processes and be explainable:

1. Sand entrainment and supply-limited transport (threshold, moisture, frost, standing water, gravel lag).
2. Dunes that are dunes: at Site (4 m) single dunes with slip faces at repose; at Local (64 m) the large forms and where a field's sand sits; at Region (1 km) sand seas with a source and a path. Dune kind must follow wind regime and sand supply.
3. What wind leaves: deflation hollows, stony lag, sand ramps, abraded soft rock if it can be done honestly.
4. Dust settling as loess downwind of its sources.
5. Dry basins: playa floors, salt crust, old shorelines.

Wind direction and strength come from the model's climate, never from a constant.

## 2. State now

Works, verified only on hand-built beds through a private dev harness (not yet in the module build, no tests, no tool):

- `constant/Wind.hpp` — grain, thresholds, shadow and climb slopes, lag armour, damp constants.
- `type/Regime.hpp`, `fn/Regime.{hpp,cpp}` — the sand rose (16 sectors, m²/yr per metre of width a dry saturated flat bed passes per sector). `regimeAtLatitude` is closed form from `beltWind`, the seasonal belt shift, `Gustiness` and `storminess`; `regimeAt(seed, position, time)` reads it where the landscape reads the planet (`planetPoint`); `regimeOf` builds steady winds for hand-built beds; `dryShare` is the share of the year a bare surface can blow.
- `type/SandBed.hpp`, `fn/Dunes.{hpp,cpp}`, `fn/Shadow.cpp`, `fn/Avalanche.cpp` — the transport model: a slab cellular model (Werner 1995) on a torus. A slab is lifted from an open, sand-bearing cell outside the wind shadow, hops downwind, stays in a shadow always, on sand with 0.6, on a bare floor with 0.4, and at the foot of a step steeper than 45°. Every move is followed by sliding to repose (`reposeTan`, to be fed from `ReposeTan[Sand]` = 0.62 in `landscape/fn/Process.hpp`). Winds of the rose take turns in proportion to their drift (`windTurns`). Cell kinds: Open, Held (finished neighbour: sends and takes, never changes), Wet (sink), Wall (domain ring).
- Seen so far (320² cells of 4 m, one wind of 10 m/s, 250 rounds): 2 m of sand gives ridges across the wind up to 15 m high with 32 % bare floor; 0.4 m gives low isolated forms with 51 % bare floor. Sand is conserved to float rounding. Pictures: `.out/aeolian/dev-one.png`, `.out/aeolian/dev-thin.png` (not yet inspected after the last change).

Not existing yet: everything under sections 3 and 4. No shared file has been touched.

## 3. In progress

Tuning the slab model on hand-built beds (`scratchpad/dev/dev.cpp`, private; rebuild it from this file's section 7 if it is gone). Last change: single hops stray ±0.2 rad (`GustSwing`), because without it lines of cells along the wind never trade sand and the bed grows streaks along the wind. Not yet checked: that the streaks are gone, two winds giving ridges along the resultant, many winds giving stars, migration direction.

## 4. Next steps

1. Look at the pictures; tune until one wind gives crescents (thin sand) and transverse ridges (thick sand), two winds 120° apart give linear ridges along the resultant, four or more give stars.
2. `fn/Measure.{hpp,cpp}`: dune height, spacing, slip-face share, stoss and lee angle, crest orientation against the resultant, from a bed or a tile.
3. `type/Aeolian.hpp`: per-cell labels and ledger (see section 6); `fn/Wind.hpp` entry points.
4. `fn/DuneField.cpp`: bed from a `Domain` (Local and Site), run, write back; `fn/Seed.cpp`: `beginWind`.
5. Region step: `fn/Budget.cpp` (sand routing along the resultant in topological order of the wind graph, supply-limited, traps), `fn/Dust.cpp` (emission and fall-out), `fn/Basin.cpp` (terminal basins: playa cells, salt).
6. Hooks into shared files (section 6), tests `test/Aeolian*.cpp`, tool `tool/AeolianCommand.cpp` (`terra aeolian --x --z`).
7. World verification at places found by search (dry with sand supply, a river plain, a coast), pictures with `terra view`, tile-time cost.
8. Independent reviewer sub-agent, then the report with the rows for `docs/simulation.md`.

## 5. Decisions and why

- **Region is the only level with a dated sand and dust budget.** Its steps are 400 kyr and sand crosses a domain in far less, so a step is solved as a steady supply-limited flux, not as motion. Finer levels cannot hold a budget: a mismatch of 1 m²/yr between a parent's flux and a child's capacity is 1.5 km of sand per Local step. They take the parent's stock and only redistribute it.
- **Dunes are a snapshot.** A Local or Site tile holds the transport model run for a fixed span of wind work (planned: about 100 yr at Site, 40 kyr at Local) on the stock its parent hands down, on the surface its own water and slopes settled. Years per round are `slab · hop · cell / 0.6 / (drift potential · dry share)`.
- **Wind-laid sand is lifted off the ground while a finer level settles and laid back at the end.** Otherwise the slope wash of fifty settling steps flattens inherited dunes, and `raise()` answers the lost sand with rock uplift under every former crest (rock-cored dunes).
- **Free domain edges are a torus, finished neighbours are Held cells.** The torus conserves the stock exactly without tuning an inflow. Rejected: reservoir edges (the field gains sand, because a flat edge emits more than a duned interior passes).
- **Known and accepted:** dunes migrate, so a later tile's crests cannot be in phase with a finished neighbour's. Heights meet (Held cells, repose, the existing 4-cell seam blend); crest lines can end at a seam. Rejected as unaffordable: recording a neighbour's edge history, wind-ordered tile dependencies, halos wide enough for agreement (dunes need several wavelengths of fetch).
- **Rejected:** a continuum shear-stress dune model (Kroy/Sauermann) — more quantitative, far more delicate under several winds.
- **Lag armour** is state, not a function of the mixed cover: a mixed cover has no layers, so without a label every step would winnow it anew.
- **Salt** is planned as a label on the Fines class (m of evaporite at the top), not a fourth class: every process measures the surface as rock plus three classes.
- **Traps:** `BedRoughness` is taken by `hydrology/constant/Water.hpp`; all `@simulation` constants share one namespace. The evaporation demand `clamp(0.25 + 0.035·T, 0.05, 1.6)` is written inline in `Flow.cpp` and `Forcing.hpp`; `Regime.cpp` restates it a third time.
- **Closed basins today:** the flood fills every hollow to its spill and the routing takes evaporation off the discharge cell by cell, so a dry basin shows water on a one-cell path from the river mouth towards the spill, at spill level. The wetted area is right, its place and level are not; sediment settles as if the lake were full.

## 6. Contracts with other streams

Nothing is added to a shared file yet. Planned, exact-match edits only:

- `landscape/type/Terrain.hpp`: one include; one member in `Domain` (labels, regime, ledger) and one in `Tile` (labels `blown`, `loess`, `lag`, `salt`, each m and each a part of an existing class).
- `landscape/fn/Process.hpp`: one declaration, `void blowLoose(Domain& domain, FlowScratch& scratch);`.
- `landscape/fn/Replay.cpp`: one call in `stepDomain` after `cutAndFill`. On the last step of a Local or Site replay it also runs the dune field.
- `landscape/fn/Build.cpp`: one call to `beginWind(domain, setting, lookup)` (labels from the parent and finished neighbours, the regime).
- `landscape/fn/Extract.cpp`: one line copying the labels into the tile.
- `test/Main.cpp`, `tool/main.cpp`, `tool/Arguments.hpp`: one line each.

Needed from others:

- **Atmosphere:** a public closed form of the seasonal belt wind (today `BeltShift` and `BeltLag` are private to `atmosphere/fn/Season.cpp` and restated in `Regime.cpp`); wind statistics per place from the stepped model (speed distribution and directional spread by month), which would replace the assumed Gaussian spread. The tracer interface being added for volcanic ash should carry desert dust: this stream can hand it an emission field.
- **Routing owner:** runoff should vanish on thick sand (dune fields carry no channels); terminal lakes should pool at the basin floor at their balance level.
- **Ground level:** must not rill wind-laid sand; could carry ripples from the tile's regime.
- **Coasts:** dry beach sand is ordinary Sand in the cover; nothing else is needed.
- **Renderer:** materials for salt crust, loess and stone pavement, read from the tile labels.

## 7. How to build, run and verify

Module build (shared build directory, other streams use it too):

```
mise -C @simulation run build
mise -C @simulation run test
.out/simulation/build/macosx/arm64/release/terra <command>
```

Private dev harness against the existing library, so the shared build is not disturbed:

```
SIM=@simulation
clang++ -std=c++20 -O2 -I $SIM/src dev.cpp $SIM/src/aeolian/fn/{Regime,Dunes,Shadow,Avalanche}.cpp \
  .out/simulation/build/macosx/arm64/release/libderivean-simulation.a -o dev
OUT=.out/aeolian ./dev <name> <sand m> <rounds> [cell m] [winds] [degrees between two winds]
```

`dev.cpp` builds a flat 320² bed, runs `blowBed` and writes a hillshaded `dev-<name>.png`. Pictures and private outputs go to `.out/aeolian/`.

## 8. Open questions

- How far the slab model's coarsening can be trusted as a clock: after about 200 yr of strong wind it has gathered a 2 m sheet into one ridge per domain width.
- Cost: 250 rounds on 320² cells took 3.8 s on the loaded machine. A site tile's present cost is not yet measured.
- Whether old shorelines are cut here or by the lake-shore stream from a high-stand level this stream supplies.
- Whether a far-field dust source beyond a Region domain should be a closed form over the planet grid or wait for the atmosphere's tracer.
