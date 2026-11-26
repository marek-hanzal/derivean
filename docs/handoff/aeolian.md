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
| `type/Regime.hpp`, `fn/Regime.{hpp,cpp}` | the sand rose: m²/yr a dry saturated flat bed passes per sector (16). `regimeAtLatitude` is closed form from `beltWind`, the seasonal belt shift, `Gustiness` and `storminess`; `regimeAt` reads it where the landscape reads the planet; `regimeOf` builds steady winds for hand-built beds; `dryShare`; `aridityOf` (1 − rain / evaporation demand; the demand is `hydrology/constant/Water.hpp`'s) |
| `type/SandBed.hpp`, `fn/Dunes.{hpp,cpp}`, `fn/Shadow.cpp`, `fn/Avalanche.cpp` | the transport model on a bare bed (below) |
| `fn/Measure.{hpp,cpp}` | `measureDunes`: sand share, slip-face share, windward and lee slope, crest angle to the wind, order, height and spacing across the crests, bodies of sand |
| `type/Aeolian.hpp` | `WindLabels` (blown, loess, lag, salt), `WindLedger`, `WindField` (Domain member), `WindRecord` (Tile member) |
| `fn/Wind.hpp` | every entry point and the helpers shared between the files below |
| `fn/Seed.cpp` | `beginWind`: regime, labels from parent and finished neighbours, wind-laid sand parked off the ground at Local and Site; `windRecordOf` |
| `fn/Cover.cpp` | `underFlood`, `heldByWater`, `mobileSand` (armour rule), `takeSand` (lag grows, fines leave as dust), `layBlown`, `bookWind`, `tidyLabels` |
| `fn/DuneField.cpp` | `shapeDunes`: Local and Site, on the last step of the replay |
| `fn/Budget.cpp` | `blowLoose` (the hook), `lastStep`, `windGraph`, `routeSand`: the Region step, and parking between Region steps |
| `fn/Dust.cpp` | `settleDust`: Region; lays the far dust too |
| `fn/FarDust.cpp` | `readFarDust` (called by `beginWind`): Region, `WindField::farDust`, a closed form over the planet grid of the dust that deserts beyond the domain send |
| `fn/Soak.cpp` | `sinkIntoBed`: transmission losses of dry-country beds, a hook for the routing (not yet called; section 6) |
| `fn/Basin.cpp` | `dryBasins`: Region, salt in basins whose inflow the full lake surface evaporates |
| `tool/AeolianCommand.cpp` | `terra aeolian --x --z [--level] [--span]` statistics and a map; `terra aeolian --find dry` |

**The transport model** (`blowBed`) is the slab model of Werner (1995) in flux form, on a torus. Per round: every open, sand-bearing cell outside the wind shadow gives a slab (more where it faces the wind over a hop's length, up to a slope of 0.2; more where it stands above its four neighbours; times a seeded gust); what is in the air moves one hop and comes down entirely at the first shadowed cell on its way, else 60 % on sand and 40 % on a bare floor, and not at all up a step steeper than 45°; then one sweep of sliding to repose (`ReposeTan[Sand]` = 0.62 from `landscape/fn/Process.hpp`). Winds of the rose take turns in proportion to their drift. Cell kinds: Open, Held (finished neighbour: sends and takes, never changes), Wet (sink), Wall (domain ring). Deterministic from the bed and its seed.

Seen on hand-built beds of 320² cells of 4 m, winds of 10 m/s (`.out/aeolian/dev-*.png`): one wind over 6 m of sand gives ridges across the wind with Y-junctions, up to 20 m, 23 % of the sand on slip faces, steepest 33.9°, windward mean 15° (`dev-thick.png`); 1 m gives isolated mounds with a concave lee (`dev-scarce.png`); two winds 120° apart give ridges along the resultant (`dev-two120.png`); four winds give short ridges and peaks with no preferred direction, not clean stars (`dev-four.png`). Cost: 4.5 ms of CPU per round.

**Per level.** Region: each step routes mobile sand along the resultant drift in topological order of the wind graph. Capacity = resultant × dry share × step × (1 − green / 0.3)², where green = rain over evaporation demand: plants hold sand from about 300 mm in a hot desert, so a desert's wetter margin keeps what reaches it. Sinks: water; streams by `Q / (Q + 3e7)` for the share of the year they run (2 % in the driest country, rising with rain, and at least `Q / (Q + 1e9)` for rivers from wetter ground); and the room under a 6° lee line and a 4° ramp line, which start only at a break steeper than 20° over one cell and carry on over every cell they pass, filled ones included. Dust is raised (winnowed fines, fines struck by passing sand, silt of depositing river bars) and settled down the same graph (two sizes, 60 km and 600 km, sooner in rain); the far dust (`farDust`, read once per tile) settles on every Free cell each step, times rain wash-out, times (1 − aridity)² for what the ground keeps. Salt is laid level from the lowest ground of terminal basins, inside the lake that would balance the inflow. Between its steps wind-laid sand and loess are parked off the ground (not on water, ice or channel cells, whose wind labels are cleared) and laid back at the start of the next `blowLoose`; the last step keeps them on the ground. Local and Site: nothing during settling; on the last step the parked sand is laid back and `blowBed` runs for the level's span (Site 3 kyr, Local 40 kyr of the regime's drift potential times the dry share, at most 500 rounds), spread over the pool `buildDomain` gives `WindField::workers`. Ground: nothing.

**Far dust** (`FarDust.cpp`): at nodes 64 cells apart, the air is followed back along the annual mean wind of `regimeAt` in 30 km stretches up to 2400 km or until the mean wind is calm. Each stretch outside the domain's square raises `5e-5 m/yr × aridity⁴ × min(2, drift potential / 30 m²/yr)` (planet precipitation, temperature, land above sea level), integrated exactly over the stretch against fall-out at `DustFall` sped up by rain. Settling = Σ flux / fall length. Cost about 4 ms per node.

**Dune pass on a pool** (`Dunes.cpp`): per round the lift is per row; the scatter runs in an even number of fixed row bands at least twice the farthest landing (`hop × 1.4 + 0.7 × Deflect + 2` rows), even bands together then odd ones, each with its own ledger sum. The cut depends on bed size and hop only, so the field is bit-identical for any pool, none included (test). Shadow, slides and landing stay serial; they are under a tenth of a round.

**Which side of dust is built:** the geological side (loess laid during Region tile building). The calendar side — a dust storm during play as an `AirSource` for `addSource` in `atmosphere/type/Plume.hpp` — is not built; see section 6.

## 3. In progress

Third worker, 26 Nov. Pictures and logs in `.out/aeolian/w/` (`high/` the lat-31° tile, `trade/` the trade-wind desert); nothing is half-edited.

**Why the trade-wind desert has no loose ground** (Region x 5498827 z 3401024, lat −24°; a scratch replay of `stepDomain` that measures the mean cover of the Free cells before and after each process, every 10th step):

- Climate: mean rain 0.27 m/yr, but half the cells under 0.1 m; 18.5 °C; runoff mean 0.069 m/yr; 61 % of the ground under 1 % slope; a trunk river of 2e10 m³/yr runs through to the sea; 0.3 % flooded.
- Supply is not the limit: `shapeSlopes` turns 0.25–0.29 m of rock a step (0.7 m/Myr) into cover (0.10 coarse, 0.13 sand, 0.04 fines; most of it creep, outside the `weathered` ledger, which books 0.003–0.008 m).
- `cutAndFill` takes 99–100 % of it again in the same step, every step, from every cell; cover at the end of a step is 0.02–0.04 m of sand. Rivers export the lot (rivers' `sedimentOut` dwarfs everything; deposition over the whole run 0.2 m).
- What the wind gets after that: it lifts 6 mm a step; of what it lifted over the run 76 % drowned (sea, lakes, streams), 20 % left the tile, 13 % stayed. Running the wind before `cutAndFill` changes nothing: what it parks is safe, but it parks only what it laid, and it lays little.
- Transmission losses (the `sinkIntoBed` hook wired into a copy of `routeWater`): 15 % of the runoff sinks (0.010 of 0.069 m/yr); cover unchanged (0.043 against 0.039 m), because the Region strips cover with the streams each cell hides, whose pace follows the cell's own runoff, not the routed discharge. Taking a share of the own runoff off as infiltration (sheet over loose ground sinking within ~300 m) also changed nothing: it reads the cover the previous step left, which is already gone.
- The lever that does work, tried in a copy of `Fluvial.cpp`: in `measurePower`, below a channel head, cover erodibility × (1 − aridity) ("in dry country the streams a cell hides die out inside it and leave their load there"). Sand cover 0.04 → 0.14 m mean, cells with more than 5 cm of cover 0.6 % → 37 %, sand the wind lifts over the run 0.66 → 9.9 m, river deposition 0.2 → 5.7 m. Still no erg on this tile: what the wind lifts goes to the sea (40 %) or off the tile (60 %); nothing on land holds it. Sent to the lead as a proposal for the relief owner (section 6).
- So the defect is the Region's cover wash, not weathering, sheltering or the order of the hooks. Ergs then still need a trap: a basin, a plant margin (now in), or a river that ends inland.

**Changed in this run.**

- Shelter lines start only at a break steeper than 20° over one cell and continue over every cell they pass (a filled one too); before, every rise sheltered its lee and at 1 km cells whole ranges were shelter. On the lat-31° tile (x 15101986 z 12099507) sand lifted over the run (m over the domain) was 2.3 with the old lines, 3.9 with breaks only, 1.2 with breaks and plant hold; 92–99 % of it drowns in every variant. Wind-laid sand covers 0.3 % of the ground against 1.8 % with the old lines, which held it as shelter of every hill rather than as dunes.
- Streams catch blown sand only while they run. Plants hold sand where rain over demand passes 0.3.
- Far dust (`FarDust.cpp`, above). Planet map `.out/aeolian/w/fardust-planet.png` (red emission, green deposition, blue sea; plumes run along the belts). Deposition peaks at 2.2e-4 m/yr inside the largest deserts; 1e-5–1e-4 m/yr on 3 % of the map, under 1e-7 on 82 %. On the trade-wind tile it leaves 2.4 m of loess as a mean, ≥ 1 m on 50 % of the ground, up to 45 m; on the lat-31° tile 0.05 m.
- `sinkIntoBed` (section 6). Dune pass on a pool (above): 320² bed, 150 rounds, 1.43 s alone, 0.97 s on 8 threads of a loaded machine, identical bits.
- Tests: the budget ridge is an escarpment (700 m); the basin check compares the salt with the floor of its own lake only (the coast stream's shore work now splits the test hollow into a 1591-cell lake and a 7-cell pool); far dust: none from a wet world, some everywhere from a desert world and never more than deserts raise; the pool check runs two slanting winds with held cells and compares the ledger too.

**Pictures with the final code** (`.out/aeolian/w/high/`, `.out/aeolian/w/trade/`):

- Region, lat 31° (`high/aeolian-region.png`): wind-laid sand on 0.3 % of the ground, thickest 35 m at x 15118848 z 12185088; mixed cover 0.4–1 m on the ranges; loess 0.05 m mean, up to 10 m. Trade-wind desert (`trade/aeolian-region.png`): no sand anywhere (thickest 31 m in one cell), loess 2.4 m mean from far dust.
- Local at that thickest cell (`high/aeolian-local-thick.png`): sand sheets along two edges of the tile, 46 % of the ground under wind-laid sand, 0.5 m mean, bodies up to 27 m, streaked along the wind; a bare plain between them with small ponds. A sand sheet, not a sand sea: nothing like the Namib or Great Sand Dunes.
- Site there (`high/aeolian-site-thick.png`): bare ground with stone lag on 3 %, a few sand patches up to 7 m in one corner, slip faces on 1.4 % of the sand. Reads as Sahara reg with scattered sand, which is honest for 0.016 m of sand a square; no erg anywhere on seed 42 that I have found with this code.

**Tests with the final code:** the aeolian checks, `testLandscapePlane` and `testLandscapeWorld` (budgets; tiles identical across thread counts, with the dune pass on the pool) pass. The rest of the suite was not run this time. Reviewer findings fixed: lines carried over filled cells, the pool decided in `buildDomain`, the aside clamp named for the band reach, exact far-dust integration, calm air keeps its own stretch, node count, `aridityOf` beside `dryShare`.

**Seen and not this module's:** on the lat-31° tile one salt cell holds 63–148 m of salt at x 14979584 z 12136960 (0.21 m a run ago): the pan is a deep narrow pool now, likely the same lake splitting.

## 4. Next steps

1. Once the relief owner decides on the Region cover wash (section 6): look at the trade-wind desert and the lat-31° tile again; an erg needs supply held on land, so check where the plant margin, basins and inland river ends catch it.
2. Once the routing calls `sinkIntoBed` and the groundwater takes it: check that Site dune hollows hold no ponds and that dry washes end in fans.
3. Calibrate the far dust against the Region's own emission: a uniform desert's routed `dustRaised` per area against `5e-5 × aridity⁴ × wind`; check loess thickness across a Region tile border (the halo, 32 km, is shorter than the 60 km fall of silt, so a mismatch would show as a step).
4. Look at more country: a dune field at Local whose parent has 1–5 m of sand, a playa that survives Local, a loess margin downwind of a desert.
5. Absent: yardangs; old shorelines; river bars as a renewing source during the dune pass; the calendar side of dust; glacial-age dust (the far dust uses today's planet climate for the whole window).

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
- **Shelter only behind breaks.** At 1 km cells a 6° line from every rise sheltered whole ranges, and their cover never met the wind. A line now starts only where the ground falls or rises more than 20° over one cell, and goes on over cells filled up to it. Rejected: shelter by height above the line in cell units (still shelters every range).
- **Plants hold sand.** Capacity falls to nothing as rain catches up with a third of the evaporation demand; with sand fluxes 100–1000 times under capacity nothing else on land stops sand at Region scale.
- **Far dust is a closed form, not a planet-wide pass.** Each Region reads it at a few nodes from the planet grid and today's climate, so tiles stay independent; sources inside the domain's square are left to the routed dust. What a desert receives it raises again: only (1 − aridity)² of it stays.
- **The dune pass may use the pool at Local and Site** although the rest of those tiles runs on one thread: it is one long piece at the end of a tile. `buildDomain` decides it (`WindField::workers`). Bands are cut from bed size and hop, never from the pool.
- **Transmission losses are a hook, not this module's routing.** `sinkIntoBed` only answers how much a bed takes; the routing owns discharge and the groundwater the water's fate. Rejected: a second infiltration share of the cell's own runoff here (the groundwater's `infiltrationShare` already owns that question).
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
- `landscape/fn/Build.cpp` `buildDomain`, after `domain.workers = …`: two comment lines and `domain.aeolian.workers = setting.workers;`.
- `test/Main.cpp`: declaration and call of `testAeolian`. `tool/Arguments.hpp`, `tool/main.cpp`: `aeolianCommand`.

What others can read: `Tile::aeolian.labels` (four arrays of `TileCells²`, metres, each a part of a cover class; empty on ground tiles), `.regime`, `.span`, `.rounds`, `.ledger`.

Needed from others:

- **Atmosphere:** a public closed form of the seasonal belt wind (`BeltShift` and `BeltLag` are private to `atmosphere/fn/Season.cpp` and restated in `Regime.cpp`); wind statistics per place from the stepped model (speed distribution and directional spread by month), which would replace the assumed Gaussian spread.
- **Dust, two sides.** Geological: this stream's `fn/Dust.cpp`, during Region tile building, where no atmosphere runs. Calendar: `addSource(Atmosphere&, const AirSource&)` in `atmosphere/type/Plume.hpp` (ash classes Lapilli 8 m/s, Grit 1 m/s, Dust 0.02 m/s; `ashFallAt` for what settles). A dust storm during play should be an `AirSource` whose Dust rate comes from the cells the geological side emits from (dry, `lag` below `LagArmour`, Fines at the surface, sand moving) while the weather's wind there exceeds the threshold of `saturatedFlux`; Grit is saltating sand and stays near the ground. Not built. The two stay consistent if they share source cells (tile labels), grain class (Fines) and directions (the rose of `regimeAt` is the closed form of the wind the stepped atmosphere relaxes to).
- **Transmission losses (hook for the routing, not called yet).** `float sinkIntoBed(const Domain&, size_t i, float discharge)` in `aeolian/fn/Wind.hpp`: m³/yr of the water reaching cell `i` (its own yield included) that the cell's loose bed takes in floods, at most `discharge`. Zero on cells that are not Free, under ice or standing water, where rain keeps up with evaporation demand, and on bare rock. Otherwise `aridity × (coarse + sand share of the cover) × min(1, cover / 2 m) × 5 m/yr × flood width × cell`, flood width `4.5 √(Q / (0.02 yr))` capped at the cell; parked sand counts as cover. Wash of 1e7 m³/yr: about 1 % per km; a trunk of 2e10: 0.02 % per km. Wiring proposed to the lead: in `routeWater`, in the upstream-first loop after the `Settled` override and before the open-water evaporation, `const float sunk = sinkIntoBed(domain, size_t(cell), domain.discharge[size_t(cell)]); domain.discharge[size_t(cell)] -= sunk;` and keep `sunk` per cell in a new `Domain`/`Tile` array (m³/yr) that the groundwater adds to the column's recharge (`groundwater/fn/Columns.cpp`, where `column.recharge` is set). Measured with the wiring in a scratch copy: 15 % of the runoff of the trade-wind desert sinks; alone it does not keep cover (section 3).
- **Relief owner, Region cover wash (proposal, measured in a copy of `Fluvial.cpp`):** in `measurePower`, after `cover` is computed: `if (!ground && sweep.scratch.gathered[i] < channelHeadOf(domain.key.level)) cover *= 1.0f - aridityOf(domain.rain[i], domain.warmthNow[i]);`. Effect in section 3. It changes landscape budgets and their tests, which were not run with it.
- **Groundwater / routing, one owner of infiltration:** the groundwater takes `infiltrationShare` of the runoff after the fact and the routing routes all of it; if the routing ever subtracts infiltration or bed losses itself, `Columns.cpp` must stop subtracting it again from `tile.discharge`.
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
- Parked loess is out of the water's reach between Region steps, so far dust that lands on ground that keeps it adds up over the whole window (24 Myr): a margin under 1e-5 m/yr ends with hundreds of metres unless channels cut it. Earth's loess plateaus are 100–300 m; whether the model's margins stay in that range is not checked.
- Far dust follows the annual mean wind; dust storms follow storm winds, which in monsoon and storm-track country blow elsewhere than the mean.
- Whether old shorelines are cut here or by the lake-shore stream from a high-stand level this stream would supply.
