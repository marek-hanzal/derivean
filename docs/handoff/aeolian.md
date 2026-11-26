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

All of it is in the module build and hooked into the tile replay. `test/Aeolian*.cpp` pass (private runner, section 7). Looked at on seed 42 (section 3); every other module test was run with the hooks in, one by one (section 3).

| File | What it owns |
| --- | --- |
| `constant/Wind.hpp` | grain, shadow slope (11°), climb slope, lag armour, damp constants |
| `type/Regime.hpp`, `fn/Regime.{hpp,cpp}` | the sand rose: m²/yr a dry saturated flat bed passes per sector (16). `regimeAtLatitude` is closed form from `beltWind`, the seasonal belt shift, `Gustiness` and `storminess`; `regimeAt` reads it where the landscape reads the planet; `regimeOf` builds steady winds for hand-built beds; `dryShare` (evaporation demand is `hydrology/constant/Water.hpp`'s) |
| `type/SandBed.hpp`, `fn/Dunes.{hpp,cpp}`, `fn/Shadow.cpp`, `fn/Avalanche.cpp` | the transport model on a bare bed (below) |
| `fn/Measure.{hpp,cpp}` | `measureDunes`: sand share, slip-face share, windward and lee slope, crest angle to the wind, order, height and spacing across the crests, bodies of sand |
| `type/Aeolian.hpp` | `WindLabels` (blown, loess, lag, salt), `WindLedger`, `WindField` (Domain member), `WindRecord` (Tile member) |
| `fn/Wind.hpp` | every entry point and the helpers shared between the files below |
| `fn/Seed.cpp` | `beginWind`: regime, labels from parent and finished neighbours, wind-laid sand parked off the ground at Local and Site; `windRecordOf` |
| `fn/Cover.cpp` | `underFlood`, `heldByWater`, `mobileSand` (armour rule), `takeSand` (lag grows, fines leave as dust), `layBlown`, `bookWind`, `tidyLabels` |
| `fn/DuneField.cpp` | `shapeDunes`: Local and Site, on the last step of the replay |
| `fn/Budget.cpp` | `blowLoose` (the hook), `lastStep`, `windGraph`, `routeSand`: the Region step, and parking between Region steps |
| `fn/Dust.cpp` | `settleDust`: Region |
| `fn/Basin.cpp` | `dryBasins`: Region, salt in basins whose inflow the full lake surface evaporates |
| `tool/AeolianCommand.cpp` | `terra aeolian --x --z [--level] [--span]` statistics and a map; `terra aeolian --find dry` |

**The transport model** (`blowBed`) is the slab model of Werner (1995) in flux form, on a torus. Per round: every open, sand-bearing cell outside the wind shadow gives a slab (more where it faces the wind over a hop's length, up to a slope of 0.2; more where it stands above its four neighbours; times a seeded gust); what is in the air moves one hop and comes down entirely at the first shadowed cell on its way, else 60 % on sand and 40 % on a bare floor, and not at all up a step steeper than 45°; then one sweep of sliding to repose (`ReposeTan[Sand]` = 0.62 from `landscape/fn/Process.hpp`). Winds of the rose take turns in proportion to their drift. Cell kinds: Open, Held (finished neighbour: sends and takes, never changes), Wet (sink), Wall (domain ring). Deterministic from the bed and its seed.

Seen on hand-built beds of 320² cells of 4 m, winds of 10 m/s (`.out/aeolian/dev-*.png`): one wind over 6 m of sand gives ridges across the wind with Y-junctions, up to 20 m, 23 % of the sand on slip faces, steepest 33.9°, windward mean 15° (`dev-thick.png`); 1 m gives isolated mounds with a concave lee (`dev-scarce.png`); two winds 120° apart give ridges along the resultant (`dev-two120.png`); four winds give short ridges and peaks with no preferred direction, not clean stars (`dev-four.png`). Cost: 4.5 ms of CPU per round.

**Per level.** Region: each step routes mobile sand along the resultant drift in topological order of the wind graph (capacity = resultant × dry share × step; sinks are water, rivers by `Q / (Q + 3e7)`, and the room under a 6° lee line and a 4° ramp line), raises dust (winnowed fines, fines struck by passing sand, silt of depositing river bars) and settles it down the same graph (two sizes, 60 km and 600 km, sooner in rain), and lays salt level from the lowest ground of terminal basins, inside the lake that would balance the inflow. Between its steps wind-laid sand and loess are parked off the ground (not on water, ice or channel cells, whose wind labels are cleared: what lies there is the water's) and laid back at the start of the next `blowLoose`; the last step keeps them on the ground. Local and Site: nothing during settling; on the last step the parked sand is laid back and `blowBed` runs for the level's span (Site 3 kyr, Local 40 kyr of the regime's drift potential times the dry share, at most 500 rounds, which a windy dry place always reaches). Ground: nothing.

**Which side of dust is built:** the geological side (loess laid during Region tile building). The calendar side — a dust storm during play as an `AirSource` for `addSource` in `atmosphere/type/Plume.hpp` — is not built; see section 6.

## 3. In progress

Second worker, 26 Nov. Pictures and logs in `.out/aeolian/w/`; nothing is half-edited.

**What the world showed (seed 42).**

- Region tile at x 15101986 z 12099507 (lat 31°, rain over demand ≈ 0, resultant 28 m²/yr towards 91°; `aeolian-region-a.png`): wind-laid sand on 3.7 % of the tile in the lee of ranges and in the northwest; most dry ground of the mountains carries 0.5–1 m of mixed cover the wind never touches, because at 1 km cells nearly every cell of a range lies under the 6° lee or the 4° ramp line; the large plain in the northeast is bare rock (no cover at all, a relief matter). One salt pan, 85 cells, up to 20 m.
- Without parking between Region steps no wind-laid sand survives there (best 16 km block 0.01 m); with parking on every Free cell it held 17 m over the best block; with parking only off water, ice and channels (now) 5.5 m, and loess drops from 0.019 m to 0.003 m mean because channels now cut it.
- Local at x 15024128 z 12124160 (`aeolian-local-a.png`): a sand body over the western half with a 180 m transverse ridge and a sharp lee, reg with patchy stone lag in the east. Reads as erg beside reg.
- Site at x 15032736 z 12140064 on that ridge: with the old 100-yr span 177 rounds gave cell-sized lumps (`aeolian-site-a.png`, `view-view-site-a.png`); with 3 kyr, 500 rounds give transverse ridges 5 m high, 60 m apart, slip faces on 46 % of the sand, all in the lee (`aeolian-site-b.png`, `view-view-site-b.png`). The 170 m lee face of the ridge carries avalanche ribs down the fall line.
- With the final code (parking off channels, labels cleared there; `aeolian-region-final.png`, `aeolian-local-final.png`): the Region tile has wind-laid sand on 1.7 % of its ground, up to 119 m in lee traps, and one salt pan of at most 0.21 m (the pan shrank once cells that deposition filled above the flood stopped counting as lake). Local at x 15015936 z 12132352: a sand ramp up a valley with a 140 m lee face and some 1 km round sand patches: single Region cells of sand, interpolated, that the Local pass leaves as blobs. The Site pictures above were taken before the channel rule; not repeated.
- Visible defects that are not this module's: ponds in dune hollows (`view-view-site-b.png`; runoff on thick sand, routing owner); a Local tile on the Region salt pan (x 15181000 z 12201000) comes out with no cover at all, so salt and loess labels are clipped to nothing (Local settling, relief owner); the trade-wind desert at x 5498827 z 3401024 (lat −24°) has no loose cover on dry ground, so the wind has nothing to move.

**Changed in this run.**

- `heldByWater` and `dryBasins` read standing water through `underFlood`: the flood level against the higher of the flood's surface and the ground with its parked sand. Hollows that parked sand fills were flooded on paper and drowned the sand.
- `dryBasins`: a basin is terminal when its inflow is no more than its full lake surface evaporates; salt stays within the lowest cells whose evaporation balances the inflow and below the ground that rises out of them. The routing evaporates only along the path to the outlet, so its outlet flow cannot tell (the old test failed after the routing changed).
- Region parking skips water, ice and channel cells (`ChannelHeads`) and clears their wind labels: before, a labelled cell that a river or lake later occupied was parked for ever and no river could cut loess.
- Dust decides whether it lands in water before it is laid. `measureDunes` reads heights on sand thickness. One evaporation demand (`hydrology/constant/Water.hpp`). `lastStep` is this module's one test for the last step (`coast/fn/Shore.cpp` keeps its own, with `>`).
- Test: the budget check now has a ridge, parks sand and checks ground plus parked sand changes only by what crosses the edge; the basin check asks for salt on the lowest ground of the lake rather than within a fixed radius.

**Measured.** Thread CPU of one tile replay, wind on against off, parents built beforehand, on a loaded machine: Region 6.8 s against 5.4 s; Local 6.2 s against 3.9 s; Site 4.9 s against 2.7 s (500 rounds). Taken before the reviewer fixes; those change no loop.

**Tests.** Every test of `test/Main.cpp` (except `testVolcanicLive`, whose library function was mid-edit) run one by one against the final code at about 09:50: all pass, `testLandscapePlane` and `testLandscapeWorld` (budgets, determinism) included, except `testIceSheetLocal` and `testGroundwaterTile`, whose streams were editing their sources and tests at that moment; not attributed to the wind, not re-run.

## 4. Next steps

1. Far-field dust: every Region sees only its own dust, so loess is centimetres where Earth has plateaus of 100 m downwind of deserts. A planet-scale closed form of emission and fall-out, read by each Region for the dust arriving across its edge, keeps tiles independent.
2. Look at more country once the relief stream has finished Local and Site: a dune field at Local whose parent has 1–5 m of sand, a playa that survives Local, a loess margin.
3. The dune pass costs 2.2 s of CPU per Site tile: its per-cell loops (lift, shadow, land) can go to `parallelFor`; the arrivals need a gather instead of a scatter to stay bit-identical.
4. Region mountains: at 1 km cells the lee and ramp lines shelter nearly every cell of a range; a gentler form of shelter (by height above the line in cell units, or only behind relief higher than the cell can hold) would let the wind work high deserts.
5. Absent: yardangs; old shorelines; river bars as a renewing source during the dune pass; the calendar side of dust.

## 5. Decisions and why

- **Region is the only level with a dated sand and dust budget.** Its steps are 400 kyr and sand crosses a domain in far less, so a step is solved as a steady supply-limited flux, not as motion. Finer levels cannot hold a budget: a mismatch of 1 m²/yr between a parent's flux and a child's capacity is 1.5 km of sand per Local step. They take the parent's stock and only redistribute it.
- **Dunes are a snapshot.** A Local or Site tile holds the transport model run for a fixed span of wind work on the stock its parent hands down, on the surface its own water and slopes settled. Years per round are `slab · hop · cell / 0.6 / (drift potential · dry share)`.
- **Region parks too.** Without it the slope wash and channels of the next 400 kyr took every grain the wind had laid (measured, section 3). Parking is not done on water, ice or channels: there the cover is the water's. Rejected: protecting wind sand inside the slope and river code (other owners' code, and the same effect).
- **Salt follows the lake's balance, not the routing's outlet.** Two owners now judge closure (the routing by its path, this module by area); the right fix is in the routing (section 6).
- **Site span 3 kyr.** The span is what lets the forms of a level reach the statistics they keep, a few hundred rounds from a smooth start; a dune field has stood far longer than that. A century left Site fields unformed.
- **Wind-laid sand is parked while a finer level settles and laid back at the end.** Otherwise the slope wash of fifty settling steps flattens inherited dunes, and `raise()` answers the lost sand with rock uplift under every former crest. `beginWind` takes it out of `ground`, `settledFrom` and `settledTo`; `shapeDunes` puts it back into all three.
- **Free domain edges are a torus, finished neighbours are Held cells.** The torus conserves the stock exactly without tuning an inflow. Rejected: reservoir edges (the field gains sand, because a flat edge emits more than a duned interior passes).
- **Known and accepted:** dunes migrate, so a later tile's crests cannot be in phase with a finished neighbour's. Heights meet (Held cells, repose, the existing 4-cell seam blend); crest lines can end at a seam. Rejected as unaffordable: recording a neighbour's edge history, wind-ordered tile dependencies, halos wide enough for agreement.
- **Flux form, not single slabs.** Werner's model with drawn slabs works but costs four times as much per round and leaves a surface rough by one slab per cell. In flux form the same rules act on the mean. What the draws did for it has to be put back explicitly, and each piece was found by a failure: a seeded gust per round (without it a flat bed never starts); the windward-slope gain read over a hop's length (read from cell to cell it grows a ripple three cells long); the stall above a slope of 0.2 and the crest gain (without them both sides of a dune stand at repose); sand falling at the first shadowed cell of its path (without it small dunes get no slip face, because a hop is as long as their lee); hop length and heading varying from round to round (a fixed hop resonates; a fixed heading along a grid axis leaves lines of cells that never trade sand); all aiming done before the bed changes (aiming over a half-updated bed leaned every ridge 5° one way).
- **Tried and dropped:** gain by height above the surrounding ground on the lifted slab (damps every swell) and on hop length (grows a pattern five cells long); a continuum shear-stress model (Kroy/Sauermann).
- **Labels, not classes.** `blown`, `loess`, `lag` and `salt` are parts of the three cover classes, so every process that measures the surface as rock plus cover stays right. Lag is state because a mixed cover has no layers: without it every step would winnow the same cover anew. `tidyLabels` drops the lag where water cut or laid more than its thickness in the step.
- **Wind work is entered in the tile's own ledger** (`bookWind`: `Domain::eroded`, `deposited`, `TileLedger`), or the budget check of `test/LandscapeWorld.cpp` would see ground move that nothing booked.
- **`heldByWater` reads the flood of the step** through `underFlood`: `level` against the higher of the `surface` it was found on and the ground with its parked sand. Nothing in this stream may refresh `Domain::surface` before the last wetness test; doing so made every cell the step had eroded count as flooded.
- **Closed basins in the model today:** the flood fills every hollow to its spill and the plume lays a step's sediment in it as if a lake stood to the rim, so a hollow of 160 m is full after one Region step; evaporation only thins the discharge on its way across. A terminal basin is therefore short-lived unless it is large against its sediment supply or sinking. Salt is laid while it lasts and is washed out once a river crosses it.
- **Salt is a closed form:** 1.4e-4 m³ per m³ of inflow that evaporates, never more in total than `TileLedger::dissolved` of the tile; that ledger counts the tile's own cells while the salt is counted over the whole domain with its halo, so the bound is loose by the halo.
- **Traps:** all `@simulation` constants share one namespace (`BedRoughness` is hydrology's, and only there). The evaporation demand is `evaporationDemand` in `hydrology/constant/Water.hpp`; it is still written inline in `landscape/fn/Flow.cpp`, `landscape/fn/Forcing.hpp` and `measure/fn/Earth.cpp` (not this module's files).

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
- **Routing owner:** runoff should vanish on thick sand (dune fields carry no channels; today Site dune hollows hold ponds in a hyper-arid desert); terminal lakes should pool at the basin floor at their balance level instead of filling to the spill, and a lake should lose its whole surface's evaporation, not only along the path to its outlet. Once it does, `dryBasins` can read the outlet's flow again.
- **Relief owner (Local):** a Local tile under a Region salt pan (x 15181000 z 12201000) ends with no cover at all; the pan's salt and fines go with it.
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

Every module test one by one (the suite stops at its first failure): compile each `test/*.cpp` except `Main.cpp` to an object, link them with a `main` that calls the `test…` functions named on its command line, and run each name in turn. Field dumps of a tile (rock, cover, water, labels as raw floats) and the replay with per-step wind statistics were small drivers linked the same way; none is needed to continue.

Pictures and private outputs go to `.out/aeolian/` (this run: `.out/aeolian/w/`).

## 8. Open questions

- Dune size at a given span of wind work is not calibrated against a dated field. Site fields now always run 500 rounds where the wind works, so their size is set by the round count, not by time.
- Held cells (finished neighbours) send a full slab every round while they have any sand: a saturated inflow regardless of the neighbour's stock. With 500 rounds this may show at seams; not looked at.
- Thin sheets (under about half a metre) grow low domes without slip faces within the rounds allowed.
- The drift potential near 76° latitude comes out at 235 m²/yr, five times the trades: the spread there is `StormWind` (6 m/s) at full storminess. Whether that is the model's weather or too much has not been checked against the stepped atmosphere.
- Whether real tiles hold terminal basins long enough for salt to stay: at Region yes (one pan of 85 cells on the tile looked at); at Local the pan's cover was gone.
- Whether old shorelines are cut here or by the lake-shore stream from a high-stand level this stream would supply.
