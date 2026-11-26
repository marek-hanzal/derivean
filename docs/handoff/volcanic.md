# Hand-over: volcanic

Describes the present state of the `volcanic` workstream. Rewrite it when the state changes; it is not a diary.

## 1. Goal

Own `@simulation/src/volcanic/**` (+ `test/Volcanic*.cpp`, `tool/Volcanic*.cpp`): volcanic landforms that arise from simulated emplacement at the planet's magmatic centres during a region's replay, are carried down the levels as rock, and keep living in calendar time during play. Checked against `docs/reference-landscapes.md` E1–E4: stratovolcanoes 1–3 km over a 10–30 km base with concave flanks, shields of a few degrees, calderas 2–20 km across, lava fields and plateaus, cinder cones in groups, necks. The volume built must be what the planet's record says reached the surface. Nothing placed, nothing painted by noise.

Order of work given by the lead: replay edifices → vent state, schedule and inspector data → live emplacement through the session overlay and the time jump → atmosphere coupling.

## 2. State now

Compiles with the rest of the tree. Verified only on hand-built grounds (`terra volcanic --scene …`); **not yet run on a world tile**, no tests registered yet.

Works, as seen in `terra volcanic --scene` numbers and pictures (`.out/volcanic/scene/`):

- **shield** (basalt, 8 × 60 km³): 660 m over its foot, radius 17 km, flank 2.7° top to foot; ledger closes to 1e-5 km³.
- **cone** (andesite, 6 × 60 km³): 1 340–1 530 m over its foot, radius 7–8 km, 17° at half height, 3–10° at the foot (concave); the region's own slope failure holds it near 14–17°, so the 30–35° summit cannot exist at 1 km cells and is left to the local level.
- **valley**: lava poured at a vent in a 350 m deep valley stays on the valley floor as a ribbon about 18 km long.
- **caldera**: a single eruption of 45 km³ founders a block 3.5 km in radius by 1.2 km; after one step of the region's erosion a pit 245 m deep with its rim 4 km out remains (the region's slope failure widens and fills it).
- **worn**: an andesite cone left alone loses most of its height in 2 Myr and is gone in 8 Myr.

Not working / not done: see sections 3 and 4.

## 3. In progress

- `tool/VolcanicSurvey.cpp` (every vent of the planet from its record, no tiles) and `tool/VolcanicWorld.cpp` (report and map of tiles at a place) are written and have not been built or run: the shared build was blocked by another stream at the time.
- First run on a world tile is the next thing to do; nothing about tile seams, the local level's inheritance, plugs or minor eruptions has been exercised yet.

## 4. Next steps

1. Build; run `terra volcanic --survey 1` and pick an arc, a rift and a plume province from its list; run `terra volcanic --x … --z …` there (one region tile first) and look at the map and at `terra view`. Fix what the first world tile shows (cost per tile, seams, sizes).
2. Local level: check `--scene cinder` and `--scene summit` (minor eruptions: cinder cones with short flows on basalt, summit cone and flank domes on andesite), then a local tile under a real edifice. The summit cone at 30–35° and its crater must come from here.
3. Tests `test/VolcanicLaws.cpp`, `test/VolcanicWorld.cpp` (register in `test/Main.cpp`): shield vs cone, valley, ledger, caldera, shared edge of two neighbouring tiles, determinism 1 thread vs pool.
4. Voxel side, now mine (the ground stream is closed): clamp datum to `rock − pile` in `simulation/fn/Query.cpp` (`readSurface`) and `relief/fn/Ground.cpp` (`GroundLattice::at`), carry `pile`/lava/vent in `GroundLattice`, `neckAt` then `pileBedAt` in `relief/fn/Form.cpp` (`formColumn`), the same in `ground/fn/Partings.cpp`; extend the alignment test (`materialAt` equals `sampleBlock`) with points in a pile and in a neck. Columnar joints: `columnJointDistance(seed, body, across, width)` exists in `rockmass/type/RockMass.hpp`; use it for `PileBed::sheet` beds (colonnade 0.3–0.6 m in the lower two thirds, 0.1–0.2 m above) and for plugs (radial frame).
5. Caldera walls at the local level (see section 5, "Caldera").
6. Rows for `docs/simulation.md` (send to the lead), independent review, report.
7. Vent state in calendar time: `ventUnrest(seed, vent, calendarTime)` → chamber fill, uplift, quakes per year; inspector data; the frequency report (the survey tool already prints most of it).
8. Live emplacement: run `relax` with a bound on visits per tick on live site ground; write deposits into the session overlay through an additive hook in `intervention/fn` (not designed yet); the time jump must leave the same deposits within a stated tolerance.
9. Atmosphere: hand `AirSource`s to `addSource` (column height from eruption rate, SO2 from magma and mass), take ash thickness back from `ashFallAt`, and delete `ashFall`/`layAsh` for the live path.
10. Materials to append to `material/constant/Materials.hpp` (ids are voxel ids): loose ash/lapilli, scoria or pumice, welded tuff, fresh lava by composition; `Magma` exists for the molten state.

## 5. Decisions and why

- **One law of motion** (`fn/Relax.cpp`): a mobile layer on a height grid moves to lower neighbours while `fall × min(thickness, sheet) > yield(heat)`; heat falls with distance travelled (faster under water) and the layer sets where it is spent. Lava, hot ash currents and loose fall (a layer with a fixed angle of rest and no cooling) are the same law with different numbers (`constant/Rheology.hpp`). `relax` takes a bound on visits so play can advance a flow tick by tick; the replay runs it to rest. The local solve is exact levelling of a cell with its lower neighbours; the first version moved half of an excess per visit and was five times slower.
- **Emplacement works on `Emplacement`** (`type/Emplacement.hpp`), plain heights with `Open`/`Passing`/`Closed` cells, not on `Domain`: the same code must run on live ground. `fn/Lay.cpp` is the only place that maps a domain onto it.
- **Rejected**: stamping cones; routing parcels down steepest descent (not time-steppable, and chaotic at lobe scale across tile edges); a per-vent patch simulated once per world (it could not see the tile's own erosion); a pile stored as thickness in `Ground` (erosion is done by other owners' code and would not keep it in step).
- **Pile** = per cell the depth of its floor below the datum (`VolcanicDomain::floor`); thickness is `rock − (datum − floor)`. It needs no bookkeeping when other processes lower the rock or uplift moves rock and datum. A tile hands down thickness (`VolcanicTile::pile`), which interpolates; a child turns it back into a floor.
- **Fresh volcanic rock barely erodes** (`fn/Pile.cpp`, `LavaSealing` 2 Myr, `AshSealing` 0.3 Myr): erodibility and weathering are scaled by `age / (age + sealing)`. Reason: the region's fluvial law lowers every cell at the pace of a headwater stream on the cell's slope whatever the rain; with basalt's erodibility that halves any unsupported relief within one 400 kyr step, so without this an edifice is gone in the step that built it. Physically it stands for lava and scoria swallowing the rain until weathering seals them. Finer levels multiply by `min(1, age / span)` as well, because they settle for a fixed span whatever the age of the ground.
- **Isostasy** (agreed with the lead): the planet's `thickening` already raises the surface for all melt, so under a newly laid pile rock and datum sink by the pile's full thickness spread by two box passes of 14 cells (`fn/Settle.cpp`). Do not touch `thickening`.
- **Budget**: per planet cell and mark interval, `erupted(mark k+1) − erupted(mark k)` × cell area, dealt to the centres inside the cell by vigour (`fn/Source.cpp`). Cells with melt but no centre inside lose their volume; the survey prints the dealt share.
- **Schedule**: steps of a vent are awake in spells of four region steps (70 % awake) with log-normal unevenness 1.2; the conduit moves every 2–7 steps within 6 km (andesite) or 10 km (basalt) of the centre. Single eruptions are a seeded sequence along the axis of supplied volume, in runs of 256 that sum exactly (`fn/Chamber.cpp`), so the eruptions around any date are found without counting from the start and the replay and the calendar share one history. Medians 0.01 km³ (basalt) and 0.03 km³ (andesite), log-normal spreads 1.3 and 1.5.
- **Caldera**: a single eruption above 2 km³ (basalt) or 12 km³ (andesite) founders a block 500 m or 1 200 m deep, as wide as its volume requires, at the END of its step. At 1 km cells the region's slope failure (which lets a kilometre-wide cell stand at 0.45 of a rock face) turns the ring wall into a 15° funnel. Planned and not built: the local level re-draws the wall as narrow as its own cells allow by applying `founder(local) − spline(founder(region))`, only where the pile's top is not younger than the collapse.
- **Region styles**: basalt 97 % lava, 3 % ash (its scoria is the local level's cinder cones); andesite 40 % lava, 20 % fall around the vent, 20 % block-and-ash currents, 20 % ash; a chamber-emptying eruption 5 % fall, 60 % ash currents, 35 % ash. Large basalt outpourings travel as inflated sheets (sheet thickness × √(km³ per part), at most 5): plains instead of shields. Flows within one eruption get 20–100 % of the cooling length in turn, which is what makes a flank instead of a pancake.
- **Minor eruptions** (`fn/Minor.cpp`): 3 % of every episode younger than the local level's settling time is left out by the region and built by the local level as dated small eruptions (cinder cone + flow, dome, summit cone), each in the settling step that lies its age before the end. Their lava sets within 1.7 km so both sides of a local tile edge see a flow whole (local halo = 2 km).
- **Traps**: `Conduit` is taken by `groundwater/type/Cave.hpp` (ours is `Feeder`). `sed -i` on macOS has no `\|`. `domain.level` is stale after the last step's cutting; reflood before drawing water. A region-level `PlaneSpec` plane loses 20–140 m per step by itself (the region's hidden-stream lowering); scenes use granite and judge form, not survival. Thin deposits (< 2 m in a step) stay loose cover and are not pile.

## 6. Contracts with other streams

**Rock above the datum (contract v1, approved by the lead).** `Tile::volcanic` (`std::shared_ptr<const VolcanicTile>`, null where a tile holds nothing volcanic), `Domain::volcanic` (`VolcanicDomain`, arrays allocated lazily). Readers (`volcanic/fn/Volcanic.hpp`):

```cpp
PileColumn pileColumn(const Tile& tile, size_t cell);          // {thickness, laid, lava, vent}; zeros where none
PileBed pileBedAt(uint64_t seed, const VolcanicTile& volcanic, const PileColumn& column, float height);   // height above the floor = y − (rock − thickness)
Material neckAt(const VolcanicTile& volcanic, Vec2 position);  // plug of a conduit at any depth; Air outside
```

**Lines added to shared files** (all present):

- `landscape/type/Terrain.hpp`: `#include "volcanic/type/Pile.hpp"`; `Tile::volcanic`; `Domain::volcanic`.
- `landscape/fn/Process.hpp`: `void erupt(Domain& domain, FlowScratch& scratch, const struct Setting& setting);`
- `landscape/fn/Replay.cpp`: include; `erupt(domain, scratch, setting);` after `raise` in `stepDomain`; `tile->volcanic = volcanicOf(domain);` after `readClimate` in `replayTile`.
- `landscape/fn/Build.cpp`: include; `beginVolcanic(domain, setting, lookup);` after `openRim` in `buildDomain`.
- `landscape/fn/Units.cpp`: include; `if (volcanicUnit(domain, i, seed)) continue;` first in the loop of `refreshUnits`.
- `tool/main.cpp`, `tool/Arguments.hpp`: the `volcanic` command.

**Planet inputs** (lead's, in place): `centreNear(planet, position)` → `MagmaticCentre{position, cellX, cellZ, vigor}`; `CrustColumn::erupted`, `CrustColumn::meltSource` (`Arc` → andesite, `Rift`/`Plume` → basalt), `PastMark::erupted` at every region anchor; `CrustColumn::meltTime` (start of the planet step of the last melt: supplied now if `meltTime + PlanetStep ≥ planet.time`).

**Rock mass** (lead's): `columnJointDistance(seed, body, across, width)`; not used yet.

**Atmosphere** (exists, not used yet; `docs/handoff/atmosphere.md`): `AirSource{id, position, height, start, end, ash[3] kg/s (lapilli, grit, dust), sulphur, vapour, heat}`, `addSource` before time passes the start, `ashFallAt` → kg/m² (convert with the deposit's bulk density). The near field (cone, ballistic blocks) stays ours. Pass little as dust. Injected vapour is new water unless taken from a stock.

**Intervention hook**: not designed. Needed: an additive call in the session's time stepping that lets a natural event append deposits (lava, ash thickness, removed summit) as history entries over live site tiles, replayable for `jumpTo`.

**Later**: `ventUnrest(seed, vent, calendarTime)` for the lead's time-dependent term in `geothermAt`.

## 7. How to build, run and verify

- Shared build: `mise -C @simulation run build`. While other streams are mid-save, an isolated build avoids their lock but not their half-saved files: `cd @simulation && XMAKE_CONFIGDIR=$PWD/../.out/volcanic/xmake xmake config --yes --mode=release --builddir=../.out/volcanic/build && XMAKE_CONFIGDIR=… xmake build --yes terra simulation-tests`; binary `.out/volcanic/build/macosx/arm64/release/terra`.
- Hand-built grounds: `terra volcanic --scene shield|cone|valley|caldera|worn|cinder|summit [--volume km3] [--episodes n] [--quiet steps] [--paroxysm km3] [--rain m/yr] [--slope m/m] --out dir` prints the ledger, the edifice's form and a section, and writes a map and an oblique view.
- World: `terra volcanic --survey 1` (planet only, about 20 s); `terra volcanic --x m --z m [--level 1|2] [--tiles n] --out dir --name tag`.
- Read pictures next to the catalogue's photographs (E1–E4).

## 8. Open questions

- Is the region's rate of lowering (any relief halves in a step without uplift) intended? It decides how long an extinct edifice stands; the sealing times above were chosen against it.
- Frequency of eruptions against Earth's is not yet measured (survey not run). The knobs that are physical: the planet's melt supply (lead's), the median eruption volume and its spread by magma (`fn/Chamber.cpp`), which set repose at a given supply.
- Vents under the sea are emplaced like any other (lava quenches to 12 % of its cooling length); not looked at.
