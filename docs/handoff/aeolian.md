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

All of it is in the module build and hooked into the tile replay. `test/Aeolian*.cpp` pass (run through a private runner, section 7). **Not yet verified on the real world** — no world tile had been looked at when this was written — and the module's full test run has not been repeated with the hooks in.

| File | What it owns |
| --- | --- |
| `constant/Wind.hpp` | grain, shadow slope (11°), climb slope, lag armour, damp constants |
| `type/Regime.hpp`, `fn/Regime.{hpp,cpp}` | the sand rose: m²/yr a dry saturated flat bed passes per sector (16). `regimeAtLatitude` is closed form from `beltWind`, the seasonal belt shift, `Gustiness` and `storminess`; `regimeAt` reads it where the landscape reads the planet; `regimeOf` builds steady winds for hand-built beds; `dryShare`; `evaporationDemand` |
| `type/SandBed.hpp`, `fn/Dunes.{hpp,cpp}`, `fn/Shadow.cpp`, `fn/Avalanche.cpp` | the transport model on a bare bed (below) |
| `fn/Measure.{hpp,cpp}` | `measureDunes`: sand share, slip-face share, windward and lee slope, crest angle to the wind, order, height and spacing across the crests, bodies of sand |
| `type/Aeolian.hpp` | `WindLabels` (blown, loess, lag, salt), `WindLedger`, `WindField` (Domain member), `WindRecord` (Tile member) |
| `fn/Wind.hpp` | every entry point and the helpers shared between the files below |
| `fn/Seed.cpp` | `beginWind`: regime, labels from parent and finished neighbours, wind-laid sand parked off the ground at Local and Site; `windRecordOf` |
| `fn/Cover.cpp` | `heldByWater`, `mobileSand` (armour rule), `takeSand` (lag grows, fines leave as dust), `layBlown`, `bookWind`, `tidyLabels` |
| `fn/DuneField.cpp` | `shapeDunes`: Local and Site, on the last step of the replay |
| `fn/Budget.cpp` | `blowLoose` (the hook), `windGraph`, `routeSand`: the Region step |
| `fn/Dust.cpp` | `settleDust`: Region |
| `fn/Basin.cpp` | `dryBasins`: Region, salt in basins whose water leaves only as vapour |
| `tool/AeolianCommand.cpp` | `terra aeolian --x --z [--level] [--span]` statistics and a map; `terra aeolian --find dry` |

**The transport model** (`blowBed`) is the slab model of Werner (1995) in flux form, on a torus. Per round: every open, sand-bearing cell outside the wind shadow gives a slab (more where it faces the wind over a hop's length, up to a slope of 0.2; more where it stands above its four neighbours; times a seeded gust); what is in the air moves one hop and comes down entirely at the first shadowed cell on its way, else 60 % on sand and 40 % on a bare floor, and not at all up a step steeper than 45°; then one sweep of sliding to repose (`ReposeTan[Sand]` = 0.62 from `landscape/fn/Process.hpp`). Winds of the rose take turns in proportion to their drift. Cell kinds: Open, Held (finished neighbour: sends and takes, never changes), Wet (sink), Wall (domain ring). Deterministic from the bed and its seed.

Seen on hand-built beds of 320² cells of 4 m, winds of 10 m/s (`.out/aeolian/dev-*.png`): one wind over 6 m of sand gives ridges across the wind with Y-junctions, up to 20 m, 23 % of the sand on slip faces, steepest 33.9°, windward mean 15° (`dev-thick.png`); 1 m gives isolated mounds with a concave lee (`dev-scarce.png`); two winds 120° apart give ridges along the resultant (`dev-two120.png`); four winds give short ridges and peaks with no preferred direction, not clean stars (`dev-four.png`). Cost: 4.5 ms of CPU per round.

**Per level.** Region: each step routes mobile sand along the resultant drift in topological order of the wind graph (capacity = resultant × dry share × step; sinks are water, rivers by `Q / (Q + 3e7)`, and the room under a 6° lee line and a 4° ramp line), raises dust (winnowed fines, fines struck by passing sand, silt of depositing river bars) and settles it down the same graph (two sizes, 60 km and 600 km, sooner in rain), and lays salt level from the lowest ground of terminal basins. Local and Site: nothing during settling; on the last step the parked sand is laid back and `blowBed` runs for the level's span (Site 100 yr, Local 40 kyr of the regime's drift potential times the dry share, at most 500 rounds). Ground: nothing.

**Which side of dust is built:** the geological side (loess laid during Region tile building). The calendar side — a dust storm during play as an `AirSource` for `addSource` in `atmosphere/type/Plume.hpp` — is not built; see section 6.

## 3. In progress

World verification (section 4, step 1). Nothing is half-edited. `scratchpad/dev/` holds the private harnesses; none is needed to continue.

## 4. Next steps

1. Look at real tiles: `terra aeolian --find dry`, then a Region tile at a dry place with rivers, then Local and Site where the report says sand is thickest; `terra view` there. Tune only what the pictures demand.
2. Run the module's whole test run with the hooks in (`mise -C @simulation run test`): `blowLoose` now runs in every replay, so existing budget and determinism checks see it.
3. Measure tile time with and without the wind (`terra measure performance`).
4. Independent reviewer sub-agent; then the report with the rows for `docs/simulation.md`.
5. Absent and worth building next: far-field dust from beyond a Region domain; yardangs; old shorelines; river bars as a renewing source during the dune pass; the calendar side of dust.

## 5. Decisions and why

- **Region is the only level with a dated sand and dust budget.** Its steps are 400 kyr and sand crosses a domain in far less, so a step is solved as a steady supply-limited flux, not as motion. Finer levels cannot hold a budget: a mismatch of 1 m²/yr between a parent's flux and a child's capacity is 1.5 km of sand per Local step. They take the parent's stock and only redistribute it.
- **Dunes are a snapshot.** A Local or Site tile holds the transport model run for a fixed span of wind work on the stock its parent hands down, on the surface its own water and slopes settled. Years per round are `slab · hop · cell / 0.6 / (drift potential · dry share)`.
- **Wind-laid sand is parked while a finer level settles and laid back at the end.** Otherwise the slope wash of fifty settling steps flattens inherited dunes, and `raise()` answers the lost sand with rock uplift under every former crest. `beginWind` takes it out of `ground`, `settledFrom` and `settledTo`; `shapeDunes` puts it back into all three.
- **Free domain edges are a torus, finished neighbours are Held cells.** The torus conserves the stock exactly without tuning an inflow. Rejected: reservoir edges (the field gains sand, because a flat edge emits more than a duned interior passes).
- **Known and accepted:** dunes migrate, so a later tile's crests cannot be in phase with a finished neighbour's. Heights meet (Held cells, repose, the existing 4-cell seam blend); crest lines can end at a seam. Rejected as unaffordable: recording a neighbour's edge history, wind-ordered tile dependencies, halos wide enough for agreement.
- **Flux form, not single slabs.** Werner's model with drawn slabs works but costs four times as much per round and leaves a surface rough by one slab per cell. In flux form the same rules act on the mean. What the draws did for it has to be put back explicitly, and each piece was found by a failure: a seeded gust per round (without it a flat bed never starts); the windward-slope gain read over a hop's length (read from cell to cell it grows a ripple three cells long); the stall above a slope of 0.2 and the crest gain (without them both sides of a dune stand at repose); sand falling at the first shadowed cell of its path (without it small dunes get no slip face, because a hop is as long as their lee); hop length and heading varying from round to round (a fixed hop resonates; a fixed heading along a grid axis leaves lines of cells that never trade sand); all aiming done before the bed changes (aiming over a half-updated bed leaned every ridge 5° one way).
- **Tried and dropped:** gain by height above the surrounding ground on the lifted slab (damps every swell) and on hop length (grows a pattern five cells long); a continuum shear-stress model (Kroy/Sauermann).
- **Labels, not classes.** `blown`, `loess`, `lag` and `salt` are parts of the three cover classes, so every process that measures the surface as rock plus cover stays right. Lag is state because a mixed cover has no layers: without it every step would winnow the same cover anew. `tidyLabels` drops the lag where water cut or laid more than its thickness in the step.
- **Wind work is entered in the tile's own ledger** (`bookWind`: `Domain::eroded`, `deposited`, `TileLedger`), or the budget check of `test/LandscapeWorld.cpp` would see ground move that nothing booked.
- **`heldByWater` reads the flood of the step** (`level` against the `surface` it was found on). Nothing in this stream may refresh `Domain::surface` before the last wetness test; doing so made every cell the step had eroded count as flooded.
- **Closed basins in the model today:** the flood fills every hollow to its spill and the plume lays a step's sediment in it as if a lake stood to the rim, so a hollow of 160 m is full after one Region step; evaporation only thins the discharge on its way across. A terminal basin is therefore short-lived unless it is large against its sediment supply or sinking. Salt is laid while it lasts and is washed out once a river crosses it.
- **Salt is a closed form:** 1.4e-4 m³ per m³ of inflow that evaporates, never more in total than `TileLedger::dissolved` of the tile.
- **Traps:** all `@simulation` constants share one namespace (`BedRoughness` is hydrology's). The evaporation demand `clamp(0.25 + 0.035·T, 0.05, 1.6)` is written inline in `Flow.cpp` and `Forcing.hpp` and a third time as `evaporationDemand` in `Regime.cpp`.

## 6. Contracts with other streams

Lines added to shared files:

- `landscape/type/Terrain.hpp`: `#include "aeolian/type/Aeolian.hpp"`; `WindRecord aeolian;` in `Tile`; `WindField aeolian;` in `Domain`.
- `landscape/fn/Process.hpp`: the comment line and `void blowLoose(Domain& domain, FlowScratch& scratch);`.
- `landscape/fn/Replay.cpp`: `blowLoose(domain, scratch);` after `cutAndFill` in `stepDomain`.
- `landscape/fn/Build.cpp`: `#include "aeolian/fn/Wind.hpp"`; `beginWind(domain, setting, lookup);` after `adoptNeighbours`.
- `landscape/fn/Extract.cpp`: `#include "aeolian/fn/Wind.hpp"`; `tile.aeolian = windRecordOf(domain);` in `extractTile`.
- `test/Main.cpp`: declaration and call of `testAeolian`. `tool/Arguments.hpp`, `tool/main.cpp`: `aeolianCommand`.

What others can read: `Tile::aeolian.labels` (four arrays of `TileCells²`, metres, each a part of a cover class; empty on ground tiles), `.regime`, `.span`, `.rounds`, `.ledger`.

Needed from others:

- **Atmosphere:** a public closed form of the seasonal belt wind (`BeltShift` and `BeltLag` are private to `atmosphere/fn/Season.cpp` and restated in `Regime.cpp`); wind statistics per place from the stepped model (speed distribution and directional spread by month), which would replace the assumed Gaussian spread.
- **Dust, two sides.** Geological: this stream's `fn/Dust.cpp`, during Region tile building, where no atmosphere runs. Calendar: `addSource(Atmosphere&, const AirSource&)` in `atmosphere/type/Plume.hpp` (ash classes Lapilli 8 m/s, Grit 1 m/s, Dust 0.02 m/s; `ashFallAt` for what settles). A dust storm during play should be an `AirSource` whose Dust rate comes from the cells the geological side emits from (dry, `lag` below `LagArmour`, Fines at the surface, sand moving) while the weather's wind there exceeds the threshold of `saturatedFlux`; Grit is saltating sand and stays near the ground. Not built. The two stay consistent if they share source cells (tile labels), grain class (Fines) and directions (the rose of `regimeAt` is the closed form of the wind the stepped atmosphere relaxes to).
- **Routing owner:** runoff should vanish on thick sand (dune fields carry no channels); terminal lakes should pool at the basin floor at their balance level instead of filling to the spill.
- **Ground level:** must not rill wind-laid sand (`labels.blown`); could carry ripples from the tile's regime.
- **Coasts:** dry beach sand is ordinary Sand in the cover; sand within 0.5 m above water is held damp. Old shorelines need a high-stand level per basin, which nobody supplies.
- **Renderer and queries:** materials for salt crust, loess and stone pavement, read from the tile labels.

## 7. How to build, run and verify

Module build (shared build directory, other streams use it too):

```
mise -C @simulation run build
mise -C @simulation run test
.out/simulation/build/macosx/arm64/release/terra aeolian --find dry
```

Private build that does not disturb the shared one (used so far):

```
cd @simulation
export XMAKE_CONFIGDIR=$PWD/../.out/aeolian/xmake
XM=~/.local/share/mise/installs/github-xmake-io-xmake/3.1.2/xmake
$XM config --yes --mode=release --builddir=../.out/aeolian/build
$XM build --yes simulation-tests terra
../.out/aeolian/build/macosx/arm64/release/terra aeolian --x <m> --z <m> --level region --out ../.out/aeolian
```

Only the aeolian checks (3 s of CPU):

```
printf 'void testAeolian();\nint main() { testAeolian(); }\n' > runner.cpp
clang++ -std=c++20 -O2 -I @simulation/src runner.cpp @simulation/test/AeolianDunes.cpp @simulation/test/AeolianGround.cpp \
  .out/aeolian/build/macosx/arm64/release/libderivean-simulation.a -o mytests && ./mytests
```

Pictures and private outputs go to `.out/aeolian/`.

## 8. Open questions

- Dune size at a given span of wind work is not calibrated against a dated field.
- Thin sheets (under about half a metre) grow low domes without slip faces within the rounds allowed.
- The drift potential near 76° latitude comes out at 235 m²/yr, five times the trades: the spread there is `StormWind` (6 m/s) at full storminess. Whether that is the model's weather or too much has not been checked against the stepped atmosphere.
- Whether real tiles hold terminal basins long enough for salt to stay.
- Whether old shorelines are cut here or by the lake-shore stream from a high-stand level this stream would supply.
