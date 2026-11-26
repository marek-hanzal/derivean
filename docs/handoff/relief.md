# Relief of the Region, Local and Site levels

## 1. Goal

The owner: the world must not look like a height map. A massif is one body that valleys are cut into, with faces, shoulders, debris under walls and connected crests; ground has edges that come from mechanics (rock that stands until it fails, water that cuts where it gathers, beds that differ, debris at repose); scale and statistics match real ground ([reference-landscapes.md](../reference-landscapes.md)). Owned here: the behaviour of `Level::Region`, `Local`, `Site` in `@simulation/src/landscape`, plus the tools `terra view`, `terra relief`, `terra block`.

## 2. State now

Verified (seed 42, 4.6 Gyr, on the planet as it was on 25–26 Nov; the planet has changed since, places move):

- Region maze is gone. Tile at −2467328, −4625920, `terra relief --levels 1 --crop 96`: cells above all eight neighbours 8.15 % → 3.15 %, steepest cell-to-cell fall above 60° on 0.82 % of ground → none, slope p90 26° → 16°, p99 40° → 22°, relief in 16 km windows p50 2156 → 1854 m, p90 3435 → 2850 m. Pictures: `.out/terra/relief/point1/crop-landing-new-L1.png` against `.out/terra/massif/region-crop.png`.
- Region mechanics have no grid direction of their own: `terra block --tilt 10|27|63 --level 1 --rock gneiss` puts the most frequent valley heading at 12.5° / 32.5° / 57.5°, 75–86 % of segments within 10° of the tilt, grid excess below 1.
- Slopes at Local on the humid range (5431296, 10199040, one tile, state before the last drainage change): mean 31.3°, above 30° / 45° / 60°: 58 / 8.8 / 1.0 % (before: 33.0°, 54 / 30 / 9.3 %). Site there: mean 40.6°, 10 % above 60° (before 55.6°, 54 %).
- Hand-built blocks (`terra block`, Site level): massive granite (`--sound 4`) holds a 163 m face above 60° with scree cones at 34–35° under it (`.out/terra/relief/blocks/view-n4-granite-massive-L3.png`); granite left to its slopes sheds talus at p50 34.4°, p95 36.3°; shale holds no face above 45°.

Not verified after the last edits (the shared tree did not compile for the last hour because of other streams' files; run `mise -C @simulation run test` first):

- `testLandscapePlane` — last seen failing on "a site plane was cut into gullies (5.1)" with `BareCreepRate` 0.002. The rate is back at 0.02 (the old value) and the Local/Site drainage rules are the old ones; whether that makes it pass is unconfirmed.
- `testRelief` (`test/Relief.cpp`, new) has never run to its end in the suite.
- The reviewer's fixes of 26 Nov (stale `foot`, leftover moving debris, sheet margin spelled as it acts) compiled only in part.

Known defects, with status:

| Defect | Status |
| --- | --- |
| Region maze of one-cell slots | fixed (above) |
| Grid-locked gullies on a held plane at Local and Site | cause found, not cured: the settling controller raises a blurred neighbourhood for every cut, which ribs the ground at its blur width (27 cells). Worked around by keeping the old drainage at Local and Site |
| Slopes of 60–75° over whole tiles | fixed by the strength-limited solve; re-measure on the new planet |
| Lakes (15.6 % of land, deep ponds on slopes) | outlets now drain a lake within a step at Region/Local/Site; not re-measured. The rule drains any lake whose outlet is cut below it: rock basins must be kept — not done |
| Rivers losing water at Region tile edges | not touched |
| Level consistency (same river at every level) | not touched |
| Site budgets (unbooked slope transport) | failure and debris run-out now book `failed` and `sedimentOut`; creep still discards `supply − wanted` (Hillslope.cpp, `creep`) |
| Kilometres of loose cover under Site | not investigated. Likely source now: failed rock stays as debris at Local/Site and only leaves through channels or by `wearDebris` |
| Sub-cell relief field for the next level and the renderer | not started |
| Inner-tile parallelism | not started; only `readStructure` runs through `forRows` (and `Domain::workers` is null above ground level) |

## 3. In progress

Nothing half-edited. The last change set was the reviewer's findings (see 5). The after-set of the nine places has not been run.

## 4. Next steps

1. Build and run the suite. If the Site plane check fails, the lever is `BareCreepRate` (Hillslope.cpp) and `ChannelHeads[Site]` (Process.hpp); do not touch the test.
2. Run `test/Relief.cpp` to the end; fix what it shows (thresholds were set from `terra block` runs listed in 7).
3. Reviewer finding still open: `failFaces` judges the whole column by the bed at its top, so a thin weak bed behind a strong lip is cut through the strong bed under it (Stability.cpp, the `whole` term). Give the whole-slope term the same own-bed correction as the face term, and add a weak-over-strong block case to the test.
4. Reviewer finding still open: `retreatFaces` drops the solute share at once while `failFaces` keeps it as fines; choose one.
5. Cure the controller ribs (Uplift.cpp `raise`): hold the mean over a parent cell without lifting shoulders beside a cut, then switch Local to `gathersWater` and re-run `terra measure plane --levels 23`.
6. Re-run the nine places (`before` set is old mechanics on the old planet): `terra relief --at <kind> --tiles 2 --out <dir>` for humid-range, cold-range, old-range, dry-plateau, flat-beds, fold-belt, granite, lowland, coast; then `terra view` at each.
7. Timings: `terra measure performance --x … --z …` (baseline under load, one thread: Region 3.6 s, Local 4.6 s, Site 2.4 s per tile; the measure stream's idle figures 6.3 / 4.4 / 2.6 s). The new slope solve and debris cones cost; not measured on a quiet machine.
8. Queued from the lead, not started: sub-cell relief tile field; lakes keep rock basins; rivers across tile edges; level consistency; braided valley floors (lateral planation by large rivers); canyon lands and old worn ranges present on seed 42; export the domain allocation from Build.cpp; inner parallelism with a 1-thread-versus-pool bitwise test.

## 5. Decisions and why

- Stability is not judged pair by pair. `failFaces` (Stability.cpp) visits cells lowest first; each cell carries the foot of its slope (`FlowScratch::foot`) and may stand no higher above it than its rock holds over the straight-line run (`stableDrop`), and no higher above a lower neighbour than a fresh face. Tried and rejected: tests over runs of 1, 2, 4 … 128 cells in eight directions (octagonal pyramids, axis-aligned valleys) and an incremental profile from the neighbour (a one-cell ramp at a wall's foot brought the whole wall down).
- Strength (Strength.hpp/.cpp): Culmann wedge, H ≤ (4c/γ)·sin β·cos φ/(1 − cos(β − φ)), cohesion falling with the cube root of face height. `massCohesion` (kPa at 100 m, rock jointed as usual for its kind) and `massFriction` were appended to every material definition: granite 350/34°, gneiss 300/33°, sandstone 250/32°, limestone 300/34°, schist 120/27°, shale 25/20°. With these a usual granite face is ~90 m, massive granite (×4) ~255 m, a kilometre of granite slope ~43°. First values (granite 1100 kPa) gave Local mean slopes of 44°.
- `CoarseFlank` 0.45: a cell of a kilometre holds that share of a face's stable drop (a dissected flank). With 0.6 the Region sat at 33°.
- Soundness per cell = `Domain::sound` (rockmass cohesion over the material's usual, Structure.cpp) × (1 − 0.85·weakness) / quality, clamped 0.05–4. `QualitySpan` 48 cells (was 14): the fine part of the seeded quality made every cell erode at its own pace. `FractureSpacing` is 0 above ground level: fractures come from `fractureShare`.
- Debris: rock that breaks stays on its cell in the rock's weathering classes (Local, Site) or goes into the cell's water as load in the same step and is booked as erosion (Region, `FlowScratch::delivered`; without the booking the plate did not rebound and the land sank under the sea). `settleDebris` (Talus.cpp) finds the resting surface lowest cell first, runs what stands above it down slopes at ≥ 0.95 of repose and lays it as a cone (`heap`). Tried and rejected: pairwise sweeps (talus at 50–70° after 40 sweeps) and depositing the whole parcel on the first gentle cell (heaps 80–175 m high on one cell).
- Free faces retreat by weathering (`retreatFaces`): a column keeps how far its face has gone (`FlowScratch::retreated`) and comes down when that is a cell's width, leaving a ramp at `RampTan` 0.75. Lowering the column a little every step turned every wall into a ramp.
- Order in `shapeSlopes` above ground level: `wearDebris`, `breakSlopes`, `weather`, `creep`, `settleDebris`. Weathering first left 0.4–1.2 m of waste on every face, so no face counted as bare and shale never failed.
- A mantle of debris (over max(1 m, 0.1 cell) and resting at ≤ `RestingTan` 0.8) is its own foot: the face above an apron starts anew at its top.
- Lakes: a cell under standing water whose outlet has been cut below it within the step is cut as river bed (`drained`, `pool` in Fluvial.cpp). Before, a lake lost one cell per step.
- Stream wear on a face steeper than `FreeFall` 1.19 is that of the limit, not proportional to the fall (hidden-stream lowering ate walls at 18 m per step).
- Region drainage (`gathersWater`, Process.hpp): a channel needs `ChannelHeads[Region]` 2.5e7 m³/yr of gathered water (water beyond `SheetMargin` 3.2 sheets of the slope above the cell, Flow.cpp); a cell with less is lowered as by a stream of `HiddenStreams[Region]` on its own slope; gathered water takes one neighbour by a fixed draw with the odds of the fall line; water that reaches a rim outlet leaves (the rim was a gutter that re-entered the domain). Slope² weights and shares between two neighbours both locked valleys onto grid directions.
- Local and Site keep the old drainage (all of a cell's water, threshold `ChannelHead` 3000, Site 6000, spreading by slope powers). The gathered-water rule there passed on a plane at 0° and 45° but ribbed creases and, with any floor of unchannelled lowering, changed the held plane; see defect table.
- Trap: the settling controller gains (`TrendGain` 0.5, `TrendLearn` 0.12) were tried at 1.0/0.5 and put back; they are unchanged.
- Trap: `FlowScratch` now carries state across the steps of one replay; never reuse one for another domain.

## 6. Contracts with other streams

Added to shared files (all additive unless noted):

- `material/type/MaterialDefinition.hpp`: `massCohesion`, `massFriction` (last two fields); every solid and granular definition sets them. `rockmass` reads both.
- `landscape/type/Terrain.hpp`: `Domain::sound`.
- `landscape/fn/Process.hpp`: `FlowScratch` fields `work`, `byHeight`, `spare`, `foot`, `retreated`, `left`, `soundScale`, `soundLog`, `drained`, `pool`, `travelled`, `gathered`, `delivered`; functions `breakSlopes`, `frostShare`, `orderByHeight`, `settleDebris`, `wearDebris`; constants `DebrisFrost`, `ChannelHeads` (replaces the two-way `channelHeadOf`; ground value unchanged), `HiddenStreams`, `gathersWater`.
- `landscape/fn/Build.hpp`/`Build.cpp`: `readStructure` (called after seeding for levels above ground), `QualitySpan`, `FractureSpacing` zero above ground.
- `landscape/fn/Hillslope.cpp`: `shapeSlopes` branches by level (ground path unchanged), `BareCreepRate`, `FrostCreep`, `frostShare` in `weather`.
- `landscape/fn/Fluvial.cpp`, `Flow.cpp`: see 5; the ground level takes the same branches as Local and Site except lake draining and the free-fall cap, which are off at ground level.
- `measure/type/Mosaic.hpp`: `coarse`; `measure/fn/Plane.hpp`: `allocateBare` (exported from PlaneDomain.cpp; it must list every per-cell `Domain` array).
- `tool/main.cpp`, `Arguments.hpp`: commands `view`, `relief`, `block`. `test/Main.cpp`: `testRelief` before `testLandscapeWorld`.

Step loop (`stepDomain`, Replay.cpp) is unchanged in structure: `followNeighbours`, `readClimate`, `raise`, units, `shapeSlopes`, `floodLevels`, `routeWater`, ice in cold phases, `cutAndFill`. Slope hooks of other streams belong before or after `shapeSlopes`; anything that moves loose cover should end with `settleDebris`.

Rows for `docs/simulation.md` (owned by the ground stream): slope failure and talus at Region/Local/Site — evolved, strength from a closed-form Culmann curve; free-face retreat — evolved; rock-mass soundness — seeded (`rockmass`), read once per tile; lake drainage through outlets — evolved; Region channel initiation by gathered water — evolved; `terra view` snow — closed form of the tile's climate, not a deposit.

## 7. How to build, run and verify

- `mise -C @simulation run build`, `mise -C @simulation run test` (about 1–2 min when the tree compiles). Binary: `.out/simulation/build/macosx/arm64/release/terra`.
- `terra relief --at humid-range|cold-range|old-range|dry-plateau|flat-beds|fold-belt|granite|lowland|coast [--tiles 2] [--levels 123] [--crop 96] [--out dir]` — searches the place, prints the equivalent `--x --z --site-x --site-z`, then per level: slope distribution, relief in 1/5/16 km windows, surface shares and talus angle, drainage density, valley sections, crests, peaks and pits, rms against 3×3 and 17×17 means, faces above 60°, tile ledger; writes slope-class maps. Good at Local in a high range: mean slope 25–35°, about 50 / 10 / <1 % above 30 / 45 / 60°; talus p50 32–38°.
- `terra view --x --z --above m [--to-x --to-z | --heading deg --pitch deg] [--site m --local m --region m] [--snow 0] --out dir --name tag` — perspective picture from tiles with cast shadows and haze. `--snow 0` removes the closed-form snow; glacier ice the tiles hold is always drawn.
- `terra block --level 1|2|3 --rock name [--weak name --bed m --dip r] [--sound f] [--height m --floor cells --flow m3/yr --uplift m/yr --river 0 --steps n] [--out dir --name tag]`, and `--tilt degrees` for the grid-bias check. Reference runs: `--level 3 --rock granite --sound 4 --height 220 --floor 12 --steps 40`; `--level 3 --rock granite --height 180 --river 0 --floor 30 --steps 10`; `--level 3 --rock sandstone --weak shale --sound 2 --bed 24 --height 200 --steps 40`.
- `terra measure plane --levels 23` (measure stream) is the regression for drainage: Site planes must stay under 1 m of gullies.
- Before-set (old mechanics, old planet): `.out/terra/relief/before/relief-<kind>.txt` and `slopes-<kind>-L{1,2,3}.png`. Block pictures: `.out/terra/relief/blocks/`. Local view of the landing range: `.out/terra/relief/try3/view-landing-local.png`.

## 8. Open questions

- Should `RockMass::friction`/`cohesion` be read per bed as a cell cuts down (now read once at the starting bed, planes taken as vertical)?
- What a held plane should do at Local over 6 Myr: the plane test only pins headings and the mean; real bare slopes of 20 km do dissect.
- Whether failed rock at Local should partly leave as load (as at Region) so that fast-rising ranges are not buried in their own debris.
