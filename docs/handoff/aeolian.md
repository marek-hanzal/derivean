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
| `fn/Seed.cpp` | `beginWind`: regime, labels from parent and finished neighbours, wind-laid sand parked off the ground at Local and Site; `windRecordOf`: labels clipped to the cover left, faded into a finished neighbour's at a shared edge as the ground is, and the wide square kept for later neighbours |
| `fn/Cover.cpp` | `underFlood`, `heldByWater`, `mobileSand` (armour rule), `takeSand` (lag grows, fines leave as dust), `layBlown`, `bookWind`, `tidyLabels` |
| `fn/DuneField.cpp` | `shapeDunes`: Local and Site, on the last step of the replay |
| `fn/Budget.cpp` | `blowLoose` (the hook), `lastStep`, `windGraph`, `routeSand`: the Region step, and parking between Region steps |
| `fn/Dust.cpp` | `settleDust`: Region; lays the far dust too |
| `fn/FarDust.cpp` | `readFarDust` (called by `beginWind`): Region, `WindField::farDust`, a closed form over the planet grid of the dust upwind deserts send, a function of world position alone |
| `fn/Soak.cpp` | `sinkIntoBed`: transmission losses of dry-country beds, called by the routing (section 6) |
| `fn/Basin.cpp` | `dryBasins`: Region, salt in basins whose inflow the full lake surface evaporates, laid as a level floor over the whole flood, rising at most `EvaporiteRate` (1 mm/yr) |
| `fn/Pools.cpp` | `poolEbb` (called by `windRecordOf`): Local and Site, `WindRecord::ebb`, how far a pool fed by a stream that runs part of the year stands below its spill for half the year |
| `tool/AeolianCommand.cpp` | `terra aeolian --x --z [--level] [--span]` statistics and a map; `terra aeolian --find dry` |

**The transport model** (`blowBed`) is the slab model of Werner (1995) in flux form, on a torus. Per round: every open, sand-bearing cell outside the wind shadow gives a slab (more where it faces the wind over a hop's length, up to a slope of 0.2; more where it stands above its four neighbours; times a seeded gust); what is in the air moves one hop and comes down entirely at the first shadowed cell on its way, else 60 % on sand and 40 % on a bare floor, and not at all up a step steeper than 45°; then one sweep of sliding to repose (`ReposeTan[Sand]` = 0.62 from `landscape/fn/Process.hpp`). Winds of the rose take turns in proportion to their drift. Cell kinds: Open, Held (finished neighbour: sends and takes, never changes), Wet (sink), Wall (domain ring: sand passes over it, none lands or lifts). Where no finished neighbour holds an edge, what leaves the bed downwind comes back over the upwind edge. Every read across an edge (shadow line, windward slope, crest, deflection) takes the far side's sand on this side's own edge ground, because the ground itself is not periodic. Deterministic from the bed and its seed.

Seen on hand-built beds of 320² cells of 4 m, winds of 10 m/s (`.out/aeolian/dev-*.png`): one wind over 6 m of sand gives ridges across the wind with Y-junctions, up to 20 m, 23 % of the sand on slip faces, steepest 33.9°, windward mean 15° (`dev-thick.png`); 1 m gives isolated mounds with a concave lee (`dev-scarce.png`); two winds 120° apart give ridges along the resultant (`dev-two120.png`); four winds give short ridges and peaks with no preferred direction, not clean stars (`dev-four.png`). Cost: 4.5 ms of CPU per round.

**Per level.** Region: each step routes mobile sand along the resultant drift in topological order of the wind graph. Capacity = resultant × dry share × step × (1 − green / 0.3)², where green = rain over evaporation demand: plants hold sand from about 300 mm in a hot desert, so a desert's wetter margin keeps what reaches it. Sinks: water; streams by `Q / (Q + 3e7)` for the share of the year they run (2 % in the driest country, rising with rain, and at least `Q / (Q + 1e9)` for rivers from wetter ground); and the room under a 6° lee line and a 4° ramp line, which start only at a break steeper than 20° over one cell and carry on over every cell they pass, filled ones included. Dust is raised (winnowed fines, fines struck by passing sand, silt of depositing river bars) and settled down the same graph (two sizes, 60 km and 600 km, sooner in rain); the far dust (`farDust`, read once per tile) settles on every Free cell each step, times rain wash-out, times (1 − aridity)² for what the ground keeps. Salt of terminal basins (inflow below what the whole flood surface evaporates) is `1.4e-4 ×` the step's inflow, at most what the tile's rock gave to solution, laid as a level floor across the whole flood from the lowest ground up, below the rim, the floor rising no more than 1 mm/yr above the lowest ground in a step (Death Valley core DV93-1, the fastest long-term salt pan; Bonneville's crust is ten times slower). Between its steps wind-laid sand is parked off the ground (not on water, ice or channel cells, whose wind labels are cleared) and laid back at the start of the next `blowLoose`; the last step keeps it on the ground. Loess stays among the fines for water and slopes to rework: fines they take between two turns of the wind come off the loess first (`WindField::finesLeft`), and `tidyLabels` keeps the label within the fines. Local and Site: nothing during settling; on the last step the parked sand is laid back and `blowBed` runs for the level's span (Site 3 kyr, Local 40 kyr of the regime's drift potential times the dry share, at most 500 rounds), spread over the pool `buildDomain` gives `WindField::workers`. Ground: nothing.

**Far dust** (`FarDust.cpp`): at nodes 64 cells apart, the air is followed back along the annual mean wind of `regimeAt` in 30 km stretches up to 2400 km or until the mean wind is calm. Each stretch raises `5e-5 m/yr × aridity⁴ × min(2, drift potential / 30 m²/yr)` (planet precipitation, temperature, land above sea level), integrated exactly over the stretch against fall-out at `DustFall` sped up by rain. Settling = Σ flux / fall length. The nodes lie on one world lattice (tile origins step by 256 cells, a multiple of 64) and nothing depends on the domain, so neighbouring tiles read the same field at their shared edge. Cost about 4 ms per node.

**Dune pass on a pool** (`Dunes.cpp`): per round the lift is per row; the scatter runs in an even number of fixed row bands at least twice the farthest landing (`hop × 1.4 + 0.7 × Deflect + 2` rows), even bands together then odd ones, each with its own ledger sum. The cut depends on bed size and hop only, so the field is bit-identical for any pool, none included (test). Shadow, slides and landing stay serial; they are under a tenth of a round.

**Which side of dust is built:** the geological side (loess laid during Region tile building). The calendar side — a dust storm during play as an `AirSource` for `addSource` in `atmosphere/type/Plume.hpp` — is not built; see section 6.

**Speed (27 Nov, ground-speed stream, Neon's audits 018, 025).** Bit-exact (fingerprints of every tile equal at 1.2 and 4.6 Gyr): the bed's surface is read from one array a round (`topAt` falls back to the bed only across the edge), the gust shares are constants, each band adds what enters from held cells into its own local sum and stores it once, and, in the commit with the other `std::hypot` sites, the climb test compares squares (no tile moved). `windTurns` lives in `Turns.cpp`. Windows harness, 4.6 Gyr: `blowBed` 6 382 → 5 115 s summed over 42 calls that run side by side, 0 differing calls. Numbers in the main repo's `.out/ground-speed/results.md`.

## 3. In progress

Fourth run, 26 Nov, on the lead's list. Pictures and logs in `.out/aeolian/w/`; nothing is half-edited. The cover-wash diagnosis of the trade-wind desert (the Region strips cover with the streams each cell hides, so the wind gets 6 mm a step there) still stands; the proposal is in section 6.

**Salt pan (put first by the lead).** `dryBasins` used to evaporate a basin's inflow over its 1–15 lowest cells, and one 400 kyr step laid 20–300 m on a single cell. Now the salt lies as one level floor over the whole flood, from the lowest ground up, below the rim, and the floor rises no more than `EvaporiteRate` (1 mm/yr, `constant/Wind.hpp`; Death Valley DV93-1, the fastest long-term pan; Bonneville ten times slower) above the lowest ground in a step. What does not fit stays in solution. Measured on the lat-31° tile over the full replay (scratch probe, `stepDomain` per step): worst single-step gain on one cell 80.0 m (old) → 53.5 m (new, 0.13 mm/yr over the step); the thick cell is the floor of a sinking basin filled level in each step, not a column. At Region the 1 mm/yr cap is 400 m a step and never binds; the spread does the work. `test/AeolianGround.cpp` checks both: a full step (level floor, inside the lake, under the pace) and a 2 kyr step whose salt must stop exactly at 2 m. Picture `.out/aeolian/w/base/salt/aeolian-region.png`: thickest salt 103 m after 24 Myr at x 14979584 z 12136960, 0.3 % of the ground with ≥ 2 cm.

**Site repeat (consistency's pattern).** Every Site tile had 1–2 m of loose cover in its east half under a westerly wind. Cause: `topAt` read the ground across the torus edge from the far side, so a tile whose ground falls eastwards saw a wall on its upwind edge: nothing lifted there and all sand coming round the torus stopped at it (probe of the bed, `dunebed`: west 16 columns ended with 0.01–0.15 m, the east with 0.5–1.3 m). Now a read across the edge takes the far cell's sand on the nearest cell's own ground (`Dunes.cpp`). The 2×2 Site mosaic (`.out/aeolian/w/sitepat/aeolian-mosaic.png`, x 7041024 z 13261824) has no repeat: west/east halves 1.26/1.00, 0.52/1.58, 1.41/2.47, 0.58/1.07 m; profile correlation of each tile with the first −0.47, −0.13, −0.55. Wind-laid sand on 27 % of the ground, dunes p50 1.1 m, no slip faces (a thin sheet, section 8).

**Far dust at tile borders.** The closed form left out the domain's own square, so the field stepped at every Region border by three times its mean. It now depends on world position alone: across the border between tiles 19 and 20 (row 12) the jump equals the step between two inner columns (9.9e-8 against 9.9e-8 m/yr). The loess labels are faded across the seam like the ground (`WindRecord::wide`); what still steps is the ground's own fines at the seam (west-border median 0.44 m against 0.33 inner, p90 6.6 against 3.6): the cover of two tiles built apart, not a label.

**Loess calibration.** Planet sample, 3179 land points: kept far dust (after `(1 − aridity)²`) p50 0, p90 7.5e-7, p99 2.9e-6, max 1.5e-5 m/yr. Over 2.6 Myr that is p99 7.7 m, max 39 m; over the 24 Myr Region window p99 71 m, max 364 m. Earth: the Loess Plateau holds 100–300 m over 2.6 Myr (40–115 m/Myr), European loess 1–20 m. So the model's richest margins keep 3–8 times less a year than the Plateau, but keep it 10 times longer. The 2.4 m mean the lead asked about was the trade-wind tile with the domain left out: 1e-7 m/yr, a thin European cover laid over 24 Myr. With its own desert now counted the same tile keeps 17.2 m as a mean, ≥ 1 m on 94 %, up to 126 m (`.out/aeolian/w/base/trade/aeolian-region.png`): the thickness of European loess, at 0.7 m/Myr, laid on a semi-arid desert margin while loess was parked out of the water's reach. Lead's decision: loess is no longer parked between Region steps; it stays among the fines for water and slopes (section 5). Re-measured with that and with water taking loess first (`.out/aeolian/w/loess2/`): the trade-wind tile keeps 0.32 m as a mean, ≥ 1 m on 9.5 %, up to 7.3 m, mostly on the northern hills; of 15.2 m settled over the window (domain mean) the water took 98 %. The lat-31° tile keeps 0.06 m, up to 7 m. Both are where supply and removal now balance under the Region's cover wash (section 6), which strips loess as it strips sand; with the wash proposal the balance would sit higher. The lat-31° salt pan is 138 m thick after the window (103 m with parked loess) and its thickest sand cell 61 m (46 m): fines that used to be parked now travel into the hollows.

**Where an erg could form on seed 42** (planet grid, scratch probe; map `.out/aeolian/w/basins-planet.png`). Land is 21 % of the planet, 20 % of it arid (aridity > 0.7). Closed catchments cover 0.15 % of land and hold 0.7 % of arid land; 96.9 % of arid land drains straight to the sea. Earth has some 20 % of land in endorheic basins (Tarim, Chad, Taoudeni, the Arabian interior). The one arid closed candidate (x 6515726 z 3332142, 14 cells, aridity 0.93, inflow 0.08 of its evaporation) becomes at Region a lake of 12684 cells (20 % of the tile wet, rain 0.39 m/yr, aridity 0.6). Causes, all outside this module: `shapeSurface` fills every pit of the planet surface with the load that reaches it, whatever the climate; `routePlanet` priority-floods; the Region `floodLevels` fills every hollow to its spill; the evaporation demand is low against the rain at the closed sites. This is a planet-level finding, not forced here.

**Ponds in dune hollows.** Site x 15032736 z 12140064 (aridity 0.90, 29 % under ≥ 0.5 m of wind-laid sand): 8 puddles of ≤ 6 cells, 0.24 m deep, before the loess change. With the current code (lake probe below) the two arid Sites hold 87–169 pools of ≤ 29 cells, 0.2 m deep, on stream paths.

**Arid lakes: a balance level for `floodLevels` (proposal, lead's request).** Rule: a lake stands at the lowest level h where its open-water loss `A(h) · max(0, E − P)` reaches what flows in, capped by the spill (E = `evaporationDemand`, P = rain on the lake, A the wet area below h). Measured on tiles as built (scratch probe `lakes`, lakes taken from `Tile::water`, inflow = discharge of outside cells whose main flow enters the lake; each lake lowered whole, without splitting at inner saddles; `.out/aeolian/w/lakes/`):

| Tile | aridity | E − P m/yr | lakes | inflow / spill loss < 1 | wet cells now → balance | volume |
| --- | --- | --- | --- | --- | --- | --- |
| Local 922 743 | 0.75 | 0.34 | 95 | 7 | 497 → 488 | −2 % |
| Region 24 12 | 0.60 | 0.58 | 105 | 4 | 383 → 383 | 0 |
| Region 20 12 (trade) | 0.74 | 0.67 | 38 | 0 | 353 → 353 | 0 |
| Region 57 46 (lat 31°) | 0.44 | 0.19 | 45 | 0 | 190 → 190 | 0 |
| Site 14764 11899 | 0.75 | 0.32 | 87 | – | 385 → 377 | −9 % |
| Site 14680 11855 | 0.80 | 0.40 | 169 | – | 644 → 643 | 0 |

With today's evaporation demand the rule changes almost nothing: nearly every lake gets more inflow than its spill area loses, most by 2–10× or more (rivers passing through, and Site pools on stream paths). The Local tile's pools are now 5.4 m deep on average, not 44 m: the world changed under the earlier count. With E doubled (`LAKE_E=2`; arid open water loses about 1–2 m/yr on Earth, e.g. Great Salt Lake and Lake Balkhash about 1 m/yr at mean temperatures where `evaporationDemand` gives 0.45–0.65) the Local tile loses 25 % of its wet cells and 51 % of its lake volume, its largest lake 102 → 60 cells; Region 24 12 loses 22 % of wet cells; the trade tile and lat-31° tile almost nothing. No lake dries completely in either case. So the rule is right but bites only with an arid-air evaporation (demand rising with dryness, not only with temperature), and the Site puddles are a matter of intermittent flow: their mean inflow exceeds evaporation, while real streams there run a few days a year.

**Ephemeral pools at Local and Site (lead's last item this round).** A pool fed by a stream that runs a share `running` of the year (`runningShareOf`, or `Q / (Q + PerennialFlow)` for a river from wetter ground) is full while it runs and then sinks at the gross evaporation demand; the level it holds half the year or more is `demand · max(0, ½ − running)` below the spill. Recorded as `WindRecord::ebb` per cell, not applied to `level`: a first version lowered `Domain::level` before extraction, and the reviewer showed that `Tile::water` is what Site children read to keep a hollow from being filled with rock (`landscape/fn/Seed.cpp`, `levelHollows`), what Ground's `standWater` floods again to the spill, and what the live water starts from at rest. So `Tile::water` stays the mean-flow level and the half-year water is `water − ebb` (section 6). Measured (`.out/aeolian/w/pools/`, blue kept, orange dry most of the year):

| Tile | aridity | running | largest ebb | inland water, mean flow → half the year | pools | mean depth |
| --- | --- | --- | --- | --- | --- | --- |
| Site 14680 11855 | 0.80 | 0.22 | 0.15 m | 564 → 182 cells (−68 %) | 163 → 65 | 0.15 → 0.20 m |
| Site 14764 11899 | 0.75 | 0.27 | 0.11 m | 231 → 99 cells (−57 %) | 63 → 36 | 0.18 → 0.25 m |
| Local 922 743 | 0.75 | 0.26 | 0.21 m | 497 → 471 cells (−5 %) | 96 → 89 | 5.4 → 5.6 m |

What is left at the Sites is the deepest cells of pools on stream paths, up to 18 cells. `runningShareOf` gives these climates a stream running a quarter of the year; with the few flood days of the driest country (`FloodShare`, 2 %) the ebb would be about twice as large.

**Relief's two notes.** `windRecordOf` clips every label to the cover the last step's `settleDebris` left (blown ≤ Sand, lag ≤ Coarse, salt ≤ Fines, loess ≤ Fines − salt), before and after the seam blend; the Region `tidyLabels` already clips at each step. `Budget.cpp` calls `runningShareOf`.

**Tests with the final code** (unparked loess, loess taken first, `ebb`): the aeolian checks (`mytests`, now with `checkPools`), `testLandscapePlane` and `testLandscapeWorld` pass (section 7). The rest of the suite was not run.

## 4. Next steps

1. Loess now goes with the water. Once relief decides on the Region cover wash, measure the loess margins again; they should then hold more than today's 0.3 m mean on the trade-wind tile.
2. Once the planet keeps closed arid basins (section 3, planet finding) and the routing pools terminal lakes at their balance level (section 6): look for an erg again; an erg needs supply held on land.
3. Once relief decides on the Region cover wash: the trade-wind desert and the lat-31° tile again; the routed dust is some 600 times weaker than the closed form for the same ground because of it.
4. Once the planet-side arid item lands (humidity-aware evaporation, closed basins, the balance level in `floodLevels`): measure ponds and pools again, and decide with the renderer and consistency owners whether `WindRecord::ebb` is read or folded into the water rule. Check that dry washes end in fans.
5. Absent: yardangs; old shorelines; river bars as a renewing source during the dune pass; the calendar side of dust; glacial-age dust (the far dust uses today's planet climate for the whole window).

## 5. Decisions and why

- **Region is the only level with a dated sand and dust budget.** Its steps are 400 kyr and sand crosses a domain in far less, so a step is solved as a steady supply-limited flux, not as motion. Finer levels cannot hold a budget: a mismatch of 1 m²/yr between a parent's flux and a child's capacity is 1.5 km of sand per Local step. They take the parent's stock and only redistribute it.
- **Dunes are a snapshot.** A Local or Site tile holds the transport model run for a fixed span of wind work on the stock its parent hands down, on the surface its own water and slopes settled. Years per round are `slab · hop · cell / 0.6 / (drift potential · dry share)`.
- **Region parks too.** Without it the slope wash and channels of the next 400 kyr took every grain the wind had laid (measured, section 3). Parking is not done on water, ice or channels: there the cover is the water's. Loess is not parked (lead's decision, 26 Nov): it is among the most erodible covers, and parked it built up over the whole 24 Myr window to 364 m on the richest margin; on the ground it ends where supply and removal balance. Rejected: protecting wind sand inside the slope and river code (other owners' code, and the same effect).
- **Salt follows the lake's balance, not the routing's outlet.** Two owners now judge closure (the routing by its path, this module by area); the right fix is in the routing (section 6).
- **Site span 3 kyr.** The span is what lets the forms of a level reach the statistics they keep, a few hundred rounds from a smooth start; a dune field has stood far longer than that. A century left Site fields unformed.
- **Wind-laid sand is parked while a finer level settles and laid back at the end.** Otherwise the slope wash of fifty settling steps flattens inherited dunes, and `raise()` answers the lost sand with rock uplift under every former crest. `beginWind` takes it out of `ground`, `settledFrom` and `settledTo`; `shapeDunes` puts it back into all three.
- **Free domain edges are a torus, finished neighbours are Held cells.** The torus conserves the stock exactly without tuning an inflow. Rejected: reservoir edges (the field gains sand, because a flat edge emits more than a duned interior passes).
- **Known and accepted:** dunes migrate, so a later tile's crests cannot be in phase with a finished neighbour's. Heights meet (Held cells, repose, the existing 4-cell seam blend); crest lines can end at a seam. Rejected as unaffordable: recording a neighbour's edge history, wind-ordered tile dependencies, halos wide enough for agreement.
- **Flux form, not single slabs.** Werner's model with drawn slabs works but costs four times as much per round and leaves a surface rough by one slab per cell. In flux form the same rules act on the mean. What the draws did for it has to be put back explicitly, and each piece was found by a failure: a seeded gust per round (without it a flat bed never starts); the windward-slope gain read over a hop's length (read from cell to cell it grows a ripple three cells long); the stall above a slope of 0.2 and the crest gain (without them both sides of a dune stand at repose); sand falling at the first shadowed cell of its path (without it small dunes get no slip face, because a hop is as long as their lee); hop length and heading varying from round to round (a fixed hop resonates; a fixed heading along a grid axis leaves lines of cells that never trade sand); all aiming done before the bed changes (aiming over a half-updated bed leaned every ridge 5° one way).
- **Tried and dropped:** gain by height above the surrounding ground on the lifted slab (damps every swell) and on hop length (grows a pattern five cells long); a continuum shear-stress model (Kroy/Sauermann).
- **Shelter only behind breaks.** At 1 km cells a 6° line from every rise sheltered whole ranges, and their cover never met the wind. A line now starts only where the ground falls or rises more than 20° over one cell, and goes on over cells filled up to it. Rejected: shelter by height above the line in cell units (still shelters every range).
- **Plants hold sand.** Capacity falls to nothing as rain catches up with a third of the evaporation demand; with sand fluxes 100–1000 times under capacity nothing else on land stops sand at Region scale.
- **Far dust is a closed form, not a planet-wide pass.** Each Region reads it at a few nodes from the planet grid and today's climate, so tiles stay independent. It counts every source upwind, the domain's own included: leaving out the domain's square made the field step at every tile border by three times its own mean (measured, section 3). The domain's own floor is then counted twice, by this field and by the routed dust; the routed dust is some 600 times smaller on the trade-wind tile, so the double count is negligible. What a desert receives it raises again: only (1 − aridity)² of it stays.
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
- **Transmission losses (agreed with the groundwater stream, contract in `groundwater.md` §6; relief wires it).** `float sinkIntoBed(const Domain&, size_t i, float discharge)` in `aeolian/fn/Wind.hpp`: m³/yr of the water reaching cell `i` (its own yield included) that the cell's loose bed takes in floods, at most `discharge`. Zero on cells that are not Free, under ice or standing water, where rain keeps up with evaporation demand, and on bare rock. Otherwise `aridity × (coarse + sand share of the cover) × min(1, cover / 2 m) × 5 m/yr × flood width × cell`, flood width `4.5 √(Q / (0.02 yr))` capped at the cell; parked sand counts as cover. Wash of 1e7 m³/yr: about 1 % per km; a trunk of 2e10: 0.02 % per km. `routeWater` calls it right after the karst step and subtracts its return from the discharge; the per-cell losses are extracted to `Tile::seeped` (m³/yr), which the groundwater adds to recharge without applying `infiltrationShare` on top. This stream keeps the signature, units and the bound `≤ discharge` stable. Measured with the wiring in a scratch copy: 15 % of the runoff of the trade-wind desert sinks; alone it does not keep cover (section 3).
- **Relief owner, Region cover wash (proposal, measured in a copy of `Fluvial.cpp`):** in `measurePower`, after `cover` is computed: `if (!ground && sweep.scratch.gathered[i] < channelHeadOf(domain.key.level)) cover *= 1.0f - aridityOf(domain.rain[i], domain.warmthNow[i]);`. Effect in section 3. It changes landscape budgets and their tests, which were not run with it.
- **Routing owner:** runoff should vanish on thick sand (dune fields carry no channels; today Site dune hollows hold ponds in a hyper-arid desert); terminal lakes should pool at the basin floor at their balance level instead of filling to the spill (rule for `floodLevels`, owned by consistency, and its measured effect in section 3: it bites only with an evaporation that rises with dry air), and a lake should lose its whole surface's evaporation, not only along the path to its outlet. Once it does, `dryBasins` can read the outlet's flow again.
- **Relief owner (Local):** a Local tile under a Region salt pan (x 15181000 z 12201000) ends with no cover at all; the pan's salt and fines go with it.
- **Ground level:** must not rill wind-laid sand (`labels.blown`); could carry ripples from the tile's regime. Its `standWater` (`ground/fn/Water.cpp`) floods pools to the spill with the mean flow, so a Ground tile shows a pool its Site parent's `ebb` leaves dry most of the year; the same rule (`poolEbb`) would apply there.
- **Renderer, queries and consistency: `WindRecord::ebb`** (Local and Site, TileCells², m, 0 outside pools fed by part-year streams): the water a player sees most of the year is `Tile::water − ebb`; where that is below the ground the floor is dry. `Tile::water` stays the mean-flow level that finer levels and the live water start from. A pool wider than the halo can be judged differently by two neighbouring tiles where only one sees the river through it.
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
- Whether real tiles hold terminal basins long enough for salt to stay: at Region yes (the lat-31° pan, 138 m after 24 Myr); at Local the pan's cover was gone.
- `WindRecord::wide` is clipped against the ground after `settleSeams`, while `Tile::nowWide` (what neighbours blend the ground with) is taken before it, so a neighbour fades labels and ground from slightly different covers within the 4 seam cells; the final clip keeps labels within cover. `tidyLabels` before `settleSeams` in `Replay.cpp` (relief's file) would also keep the Region labels consistent through the blend.
- `dryBasins` takes every connected standing cell as one basin, so a chain of touching lakes at different levels gets one rim and one floor.
- Ephemeral pools assume one flood season a year and no water table under the pool; several short floods keep a semi-arid pool fuller, and a water table at the floor (oases, spring-fed pans) keeps it wet. Groundwater reads `Tile::water`, not the ebb.
- Dune bed edges: a read across the torus edge takes this side's edge ground flat (`topAt`), not the slope continued. The edge lies 32 cells outside the tile, so the tile does not see it; not measured beyond the mosaic.
- Far dust follows the annual mean wind; dust storms follow storm winds, which in monsoon and storm-track country blow elsewhere than the mean.
- Whether old shorelines are cut here or by the lake-shore stream from a high-stand level this stream would supply.
