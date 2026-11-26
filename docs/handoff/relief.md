# Relief of the Region, Local and Site levels

## 1. Goal

The owner: the world must not look like a height map. A massif is one body that valleys are cut into, with faces, shoulders, debris under walls and connected crests; ground has edges that come from mechanics (rock that stands until it fails, water that cuts where it gathers, beds that differ, debris at repose); scale and statistics match real ground ([reference-landscapes.md](../reference-landscapes.md)). Owned here: the behaviour of `Level::Region`, `Local`, `Site` in `@simulation/src/landscape`, plus the tools `terra view`, `terra relief`, `terra block`.

## 2. State now

Verified (seed 42, 4.6 Gyr, on the planet as it was on 25–26 Nov; the planet has changed since, places move):

- Region maze is gone. Tile at −2467328, −4625920, `terra relief --levels 1 --crop 96`: cells above all eight neighbours 8.15 % → 3.15 %, steepest cell-to-cell fall above 60° on 0.82 % of ground → none, slope p90 26° → 16°, p99 40° → 22°, relief in 16 km windows p50 2156 → 1854 m, p90 3435 → 2850 m. Pictures: `.out/terra/relief/point1/crop-landing-new-L1.png` against `.out/terra/massif/region-crop.png`.
- Region mechanics have no grid direction of their own: `terra block --tilt 10|27|63 --level 1 --rock gneiss` puts the most frequent valley heading at 12.5° / 32.5° / 57.5°, 75–86 % of segments within 10° of the tilt, grid excess below 1.
- Slopes at Local on the humid range (5431296, 10199040, one tile, state before the last drainage change): mean 31.3°, above 30° / 45° / 60°: 58 / 8.8 / 1.0 % (before: 33.0°, 54 / 30 / 9.3 %). Site there: mean 40.6°, 10 % above 60° (before 55.6°, 54 %).
- Hand-built blocks (`terra block`, Site level): massive granite (`--sound 4`) holds a 163 m face above 60° with scree cones at 34–35° under it (`.out/terra/relief/blocks/view-n4-granite-massive-L3.png`); granite left to its slopes sheds talus at p50 34.4°, p95 36.3°; shale holds no face above 45°.

Suite (`mise -C @simulation run test`, 26 Nov, 287 s under load): Materials, Climate, Clouds, Atmosphere, Regularity, `testLandscapePlane` and `testRelief` pass up to the bedded-rock case; what runs after `testRelief` (World determinism and budgets, Inspect, Chart, …) has not been seen green with these mechanics.

- `testRelief` (`test/Relief.cpp`) covers: the strength curve, a massive granite canyon that holds a wall and no face above what its rock holds, a replay twice giving one result, slope processes conserving ground to 0.2 % of what failed, talus with a median at repose, shale holding no wall.
- `testReliefBeds` (same file) is NOT registered in `test/Main.cpp`: on sandstone over shale (24 m beds, `--sound 2`, 20 steps, river) its last check reads a steep run of 520 m on a block 200 m high. Something builds or leaves a spike there; first suspects are `heap` in Talus.cpp and the debris laid back at the end of a `settleDebris` round. Fix, then register it.
- The steepest twentieth of talus cells stands at about 50° after a step (the median is 34°): debris the run-out leaves on cells the pass had already visited.

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

Second run, 26 Nov. Landed in the tree and verified on hand-built planes (not yet on planet tiles, suite not re-run):

- **Lowering of unchannelled ground** (`measurePower`/`solveHeights`, Fluvial.cpp). Two defects found by the volcanism stream on a hand-built Region plane (345 m/Myr on a 0.2 % granite slope, the same without rain). (a) The hidden stream of a cell is now `HiddenStreams` × min(1, the cell's own runoff in m/yr): no runoff, no lowering. It must follow the cell's own runoff, never the routed discharge: tried `min(hidden, discharge)` and it rilled the top of every slope at a cell's width. (b) A mantle no longer lends its erodibility to the rock under it: the cover is stripped at its own pace and the rock cut in what is left of the step (`lowering`); the old blend by `erosionRate` gave the whole cut the cover's erodibility whenever the recent rate was small. Granite plane now: 0 without rain; 3.2 / 13 / 77 m/Myr at 0.2 / 1 / 5 % under 0.9 m/yr. `testRelief` holds the case. Applies to every level, Ground included: ground tests still to re-run.

Being tested in a private mirror, not in the tree (scratchpad `relief2/`: `lab.sh` copies the stream's files, applies `labpatch.py` switches read from `LAB_*` environment variables, builds; `driver.sh relief|beds|plane` runs the checks alone):

- The comb is the lift's kernel, not its gains: any lift that answers the deficit of a neighbourhood narrower than a slope raises shoulders beside every cut, the shoulder turns the slope's water aside and the next valley starts at its foot. A lift per parent-cell footprint (means, cubic B-spline between them) ribs a crease too, less regularly; footprints of two and four parent cells progressively stop it.
- Under the old controller the equilibrium of a held plane is full dissection down to the outlet whatever the kernel, because every stream larger than the hidden one is more erosive than the ground between. A floor under rivers the parent has drawn (gathered water ≥ the parent's channel head may not cut below `trend`) bounds that; reaches below the parent's head still grade far below the surface.
- Next in the lab: no sinking (lift ≥ 0), the Region drainage rule at Local and Site, then pictures of the landing range.

## 4. Next steps

Order set by the lead on 26 Nov:

1. The settling controller (`raise()` in Uplift.cpp). It is what ribs the ground at equal spacing and why Local and Site cannot use the Region's drainage rule. First direction to test: one correction per parent-cell footprint (16 × 16 fine cells), blended smoothly between parent cells, answering the mean over that footprint only, never an individual cut; the fine level may not move the mean of a parent cell or the bed of a river the parent has. If any controller keeps producing a wavelength, replace settling (e.g. base levels fixed along the parent's rivers, slopes rising from them at their limiting gradients) and say so. Regression: `terra measure plane --levels 23` (planes flat, a crease gathering into one valley, no comb), then `mise -C @simulation run test`. When it lands, send the lead the Local view of the landing range again (`terra view --x -2490000 --z -4600000 --above 2200 --to-x -2467328 --to-z -4625920 --site 0 --local 30000 --region 180000 --snow 0`) and one from a valley floor at eye height looking up at the range.
2. Then `gathersWater` for Local and Site (channel initiation by gathered water, hidden-stream lowering — sizes tried: Local 1e5 and 1e4 m³/yr, Site 400), then `BareCreepRate` towards 0.002; plane test after each.
3. Lakes: keep rock basins (rim of rock, outlet not cut down to the floor); today any lake whose outlet is cut below a cell loses that cell (Fluvial.cpp, `solveHeights`).
4. Reviewer findings still open: (a) `failFaces` judges the whole column by the bed at its top, so a thin weak bed behind a strong lip is cut through the strong bed under it (Stability.cpp, the `whole` term; give it the own-bed correction the face term has) — this is the stepped-wall case; (b) `retreatFaces` drops the solute share at once while `failFaces` keeps it as fines; (c) creep discards `supply − wanted`; (d) `FlowScratch` owns replay state that belongs in `Domain`; (e) `Structure.cpp` reads the nearest strata node where `Units.cpp` blends four.
5. `testReliefBeds` (see 2), then register it.
6. Tile-edge rivers, level consistency, the sub-cell relief field, inner parallelism with a 1-thread-versus-pool bitwise test, the nine places on the present planet (`terra relief --at <kind> --tiles 2 --out <dir>`, then `terra view` at each; the `before` set is old mechanics on the old planet), timings on a quiet machine (`terra measure performance --x … --z …`; under load, one thread, before: Region 3.6 s, Local 4.6 s, Site 2.4 s per tile; the new slope solve and debris cones cost and are not yet measured).
7. Also queued: braided valley floors, canyon lands and old worn ranges present on seed 42, kilometres of loose cover under Site, export of the domain allocation from Build.cpp.

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
- `tool/main.cpp`, `Arguments.hpp`: commands `view`, `relief`, `block`. `test/Main.cpp`: `testRelief` before `testLandscapeWorld` (`testReliefBeds` is defined and not called).

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
