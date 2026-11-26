# Hand-over: volcanic

Describes the present state of the `volcanic` workstream. Rewrite it when the state changes; it is not a diary.

## 1. Goal

Own `@simulation/src/volcanic/**` (+ `test/Volcanic*.cpp`, `tool/Volcanic*.cpp`): volcanic landforms that arise from simulated emplacement at the planet's magmatic centres during a region's replay, are carried down the levels as rock, and keep living in calendar time during play. Checked against `docs/reference-landscapes.md` E1–E4: stratovolcanoes 1–3 km over a 10–30 km base with concave flanks, shields of a few degrees, calderas 2–20 km across, lava fields and plateaus, cinder cones in groups, necks. The volume built must be what the planet's record says reached the surface. Nothing placed, nothing painted by noise.

Order of work given by the lead: replay edifices → vent state, schedule and inspector data → live emplacement through the session overlay and the time jump → atmosphere coupling.

## 2. State now

Compiles with the rest of the tree. Run on world tiles at the arc of seed 42 (the largest andesite vent, `--x 13686236 --z 11027538`, an island volcano by the coast). Tests `testVolcanicLaws` and `testVolcanicLive` pass through a private driver (the suite stops earlier, at another stream's test).

- **Region tile**: 15 s for one tile (`.out/volcanic/world/arc-region.txt`, `volcanic-arc-region-map.png`).
- **Local tile**: 259 s for the requested tile including the planet, the region parents and the five local tiles it waits for, on a machine at load 40 (`.out/volcanic/world/arc-local-trace.txt`, map `volcanic-arc-local-map.png`); the emplacements of the local level took 72 s of that, the slowest 1.9 s. The predecessor's run that never finished had two causes: (1) "minor" eruptions were capped in number (8 per episode) but not in size, so the largest vent built 11 km³ "minor" eruptions at 64 m cells (50 s each); now each keeps its typical size and the region keeps the rest; (2) fall landing on ground steeper than its angle of rest relaxed to a few millimetres (`Relax.cpp` slack), up to 30 s per cone; the slack is now also 0.5 % of the fall a layer holds over one cell (10–25× faster, region forms unchanged within a few metres).
- **Region against local at the vent** (scratchpad drivers `levels.cpp`, `towers.cpp`): the local surface follows the region's within about 300 m along a line through the vent. A first build showed two cells 4.5–5.5 km west of the vent under 2.8–3 km of loose cover; a later build of the same tile (with other streams' changes in the tree meanwhile) has none: deepest loose cover 116 m, at the sea edge 7.8 km from any minor vent, mean 1.2 m. Not attributed; watch for it.
- **The pile under the arc vent is 4–13 km thick** at the region (datum 3.5–11 km below sea level): 17 600 km³ in 22 Myr from a conduit that wanders within 6 km, each layer sunk by isostasy. Real arc volcanic sections are a few km to about 10 km; worth a look by the lead (it is the planet's supply dealt to one centre).
- **Sealing removed** (lead's decision): fresh volcanic rock erodes as its rock does. Scenes (`.out/volcanic/scene2/`): cone 697 m over its foot after 6 × 60 km³ (990 m with sealing), shield 474 m, worn cone gone after 10 Myr; caldera pit 114–118 m after the step that founders it (test threshold lowered from 200 to 100 m: the steep wall is the local level's). Local scenes: `summit` builds an 847 m andesite cone at 33–35° with a lava apron, `cinder` a 274 m basalt cone at 31–35°.
- **Voxel side** (predecessor's wiring, finished and reviewed here): datum clamped to `rock − pile` in `readSurface` (now with the pile interpolated by the same weights as the cover) and `GroundLattice::at` (pile kinds from the nearest node that has a pile, so margins read as their pile and not as tuff); `formColumn` reads the pile's members above its floor with their own faces and partings (recess by the member's rock), plugs cut pile and layering; the ground level reads pile beds through `volcanicUnit` (also dates the rock mass now); `ground/fn/Partings.cpp` needed no change.
- **Live eruptions, simulation side** (new, `volcanic/type/Outbreak.hpp`, `volcanic/fn/Outbreak.{hpp,cpp}`): `outbreaksBetween` gives a vent's own eruptions (the replay's sequence) in any window of era years, identical however the window is cut; `outbreakOf` gives site, style split, rates, durations, column (Mastin 2009) and lava plume (Settle 1978); `airSourcesOf` turns one into `AirSource`s; `layOutbreak` lays the near field (cone, currents, lava, caldera drop) on any `Emplacement`. Eruptions of the local level's size use its laws (short flows, cones of fragments). Figures: basalt 0.1 km³ → a day of fountaining to 4 km and 23 days of lava; andesite 0.1 km³ → 29 h column 9 km, then a dome for a year; 5 km³ andesite → 20 h at 26 km; 30 km³ → caldera, 16 h at 48 km. Nothing calls these yet (engine) and the terrain change needs an intervention edit kind (section 6).
- **Frequency** (survey, seed 42): 4.1 eruptions a year on land against Earth's 20–26 for this much land. The planet's supply on land (0.11 km³/yr reaching the surface) is the short factor; the eruption sizes (median 0.002 km³, mean 0.01–0.02 km³) are Earth-like. One supplied vent per 42 000 km² of land, neighbours 17–55 km apart; from a random place on land the nearest supplied vent is 65 km (p10) to 2 100 km (p90) away. Only physical inputs may change it: the planet's melt reaching the surface (lead's).

## 3. In progress

Nothing half-done. The temporary emplacement trace is removed from `fn/Erupt.cpp`.

## 4. Next steps

1. A 2 × 2 local run at the arc for seams and a look at `terra view` of the local edifice; watch for the kilometre columns of loose cover seen once (section 2).
2. Live terrain change: an intervention edit kind for deposits (section 6), then the engine wiring (`outbreaksBetween` for vents near the player, `addSource(airSourcesOf(...))`, `layOutbreak` when an eruption ends); unrest in the inspector (`volcanicAt`).
3. Eruptions between the region's last step and the world's age (when the age is not a multiple of 400 kyr) are counted by `ventUnrest`/`outbreaksBetween` but laid by no tile: extend the supplied steps of `Source.cpp` to the age, or let the local level lay them.
4. Caldera walls at the local level (`founder(local) − spline(founder(region))`).
5. Columnar joints for `PileBed::sheet` and plugs (`columnJointDistance`).
6. Materials: loose ash/lapilli, scoria/pumice, welded tuff, fresh lava by composition (`material/constant/Materials.hpp`, shared).
7. Rows for `docs/simulation.md` (to the lead).

## 5. Decisions and why

- **One law of motion** (`fn/Relax.cpp`): a mobile layer on a height grid moves to lower neighbours while `fall × min(thickness, sheet) > yield(heat)`; heat falls with distance travelled (faster under water) and the layer sets where it is spent. Lava, hot ash currents and loose fall (a layer with a fixed angle of rest and no cooling) are the same law with different numbers (`constant/Rheology.hpp`). `relax` takes a bound on visits so play can advance a flow tick by tick; the replay runs it to rest. The local solve is exact levelling of a cell with its lower neighbours; the first version moved half of an excess per visit and was five times slower. A surface may stand above its rest by 2 % of a flow unit or 0.5 % of the fall the layer holds over one cell, whichever is more: finer than that the grid draws no slope, and loose fall on ground steeper than its angle of rest otherwise relaxed to millimetres for tens of seconds per cone.
- **Emplacement works on `Emplacement`** (`type/Emplacement.hpp`), plain heights with `Open`/`Passing`/`Closed` cells, not on `Domain`: the same code must run on live ground. `fn/Lay.cpp` is the only place that maps a domain onto it.
- **Rejected**: stamping cones; routing parcels down steepest descent (not time-steppable, and chaotic at lobe scale across tile edges); a per-vent patch simulated once per world (it could not see the tile's own erosion); a pile stored as thickness in `Ground` (erosion is done by other owners' code and would not keep it in step).
- **Pile** = per cell the depth of its floor below the datum (`VolcanicDomain::floor`); thickness is `rock − (datum − floor)`. It needs no bookkeeping when other processes lower the rock or uplift moves rock and datum. A tile hands down thickness (`VolcanicTile::pile`), which interpolates; a child turns it back into a floor.
- **No sealing of fresh rock** (lead's decision once the region's lowering defect was fixed in 54fdbd9): volcanic rock erodes as its rock does from the step it is laid. Finer levels multiply erodibility and weathering by `min(1, age / span)` only, because they settle for a fixed span whatever the age of the ground.
- **Isostasy** (agreed with the lead): the planet's `thickening` already raises the surface for all melt, so under a newly laid pile rock and datum sink by the pile's full thickness spread by two box passes of 14 cells (`fn/Settle.cpp`). Do not touch `thickening`.
- **Budget**: per planet cell and mark interval, `erupted(mark k+1) − erupted(mark k)` × cell area, dealt to the centres inside the cell by vigour (`fn/Source.cpp`). Cells with melt but no centre inside lose their volume; the survey prints the dealt share.
- **Schedule**: steps of a vent are awake in spells of four region steps (70 % awake) with log-normal unevenness 1.2; the conduit moves every 2–7 steps within 6 km (andesite) or 10 km (basalt) of the centre. Single eruptions are a seeded sequence along the axis of supplied volume, in runs of 256 that sum exactly (`fn/Chamber.cpp`), so the eruptions around any date are found without counting from the start and the replay and the calendar share one history. Medians 0.01 km³ (basalt) and 0.03 km³ (andesite), log-normal spreads 1.3 and 1.5.
- **Caldera**: a single eruption above 2 km³ (basalt) or 12 km³ (andesite) founders a block 500 m or 1 200 m deep, as wide as its volume requires, at the END of its step. At 1 km cells the region's slope failure (which lets a kilometre-wide cell stand at 0.45 of a rock face) turns the ring wall into a 15° funnel. Planned and not built: the local level re-draws the wall as narrow as its own cells allow by applying `founder(local) − spline(founder(region))`, only where the pile's top is not younger than the collapse.
- **Region styles**: basalt 97 % lava, 3 % ash (its scoria is the local level's cinder cones); andesite 40 % lava, 20 % fall around the vent, 20 % block-and-ash currents, 20 % ash; a chamber-emptying eruption 5 % fall, 60 % ash currents, 35 % ash. Large basalt outpourings travel as inflated sheets (sheet thickness × √(km³ per part), at most 5): plains instead of shields. Flows within one eruption get 20–100 % of the cooling length in turn, which is what makes a flank instead of a pancake.
- **Minor eruptions** (`fn/Minor.cpp`): of every episode younger than the local level's settling time the region leaves out 3 % of what it brings up besides a chamber-emptying eruption (`minorVolume`), but no more than 8 eruptions of the typical size (0.08 km³ basalt, 0.4 km³ andesite); the local level builds them as dated small eruptions (cinder cone + flow, dome, summit cone), each in the settling step that lies its age before the end. Without the cap the largest vent's "minor" eruptions reached 11 km³ and a local tile took hours. Their lava sets within 1.7 km so both sides of a local tile edge see a flow whole (local halo = 2 km).
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
- Voxel side (ground stream closed, lead-approved): `simulation/fn/Query.cpp` (include; datum clamp in `readSurface`), `relief/fn/Ground.cpp` (include; `pile` read in `readGround`, datum clamp in `GroundLattice::at`), `relief/type/Ground.hpp` (`GroundPoint::pile`, `GroundLattice::pile`), `relief/type/Form.hpp` (`FormSetting::volcanic`), `relief/fn/Form.cpp` (include; pile floor, plug, pile beds, no city over a pile), `simulation/fn/Block.cpp` (`here.volcanic`), `ground/fn/Settle.cpp` (include; `volcanicUnit` first in `refreshPartings`).
- `test/Main.cpp`: `testVolcanicLaws`, `testVolcanicLive` (declaration and call each).

**Planet inputs** (lead's, in place): `centreNear(planet, position)` → `MagmaticCentre{position, cellX, cellZ, vigor}`; `CrustColumn::erupted`, `CrustColumn::meltSource` (`Arc` → andesite, `Rift`/`Plume` → basalt), `PastMark::erupted` at every region anchor; `CrustColumn::meltTime` (start of the planet step of the last melt: supplied now if `meltTime + PlanetStep ≥ planet.time`).

**Rock mass** (lead's): `columnJointDistance(seed, body, across, width)`; not used yet.

**Live eruptions** (`volcanic/fn/Outbreak.hpp`, for the engine; nothing calls them yet):

```cpp
void outbreaksBetween(uint64_t seed, const Vent& vent, double fromYear, double toYear, std::vector<Outbreak>& into);   // era years: world age + calendar seconds / YearSeconds
void airSourcesOf(const Outbreak& outbreak, double era, double ground, std::vector<AirSource>& into);              // era = Calendar::era; ground = m above the sea at the vent
NearField layOutbreak(Emplacement& field, const Outbreak& outbreak);   // fall + currents (loose), lava (rock), drop (caldera) per cell
Unrest ventUnrest(uint64_t seed, const Vent& vent, double eraYear);    // chamber fill, uplift, quakes, years to the next eruption
VolcanicRecord volcanicAt(World& world, Vec2 position, double calendarYears, double reach);   // inspector: nearest vent, its unrest, the pile
```

Vents come from the region tile under the player (`Tile::volcanic->vents`). The engine, each time the calendar crosses a window (say a year ahead), asks `outbreaksBetween` for the vents of the region tiles around the player and hands `airSourcesOf` to `addSource` before time passes their start. The near field is laid when an eruption's phases have ended, so a jump and a walk leave the same ground.

**Atmosphere** (`docs/handoff/atmosphere.md`): `AirSource{id, position, height, start, end, ash[3] kg/s (lapilli, grit, dust), sulphur, vapour, heat}`, `addSource` before time passes the start, `ashFallAt` → kg/m² (convert with the deposit's bulk density). `airSourcesOf` sends the wind-borne ash and 30 % of the hot currents as lofted ash (3 % of it as dust), sulphur dioxide 3e-3 (basalt) or 1.3e-3 (andesite) of the melt's mass, water 0.7 % or 4 %. The region replay still lays its own ash (`ashFall`/`layAsh`); the live path does not.

**Intervention hook**: not designed. Needed from the interventions owner: an edit kind that lays rock or loose deposit of given thickness per cell (and lowers for a caldera), stamped at a calendar time and replayed by `jumpTo`, so that `layOutbreak`'s result becomes part of the session's history over live site tiles. Today `Edit` only raises to a height with a `SedimentClass` fill.

**Later**: `ventUnrest` for the lead's time-dependent term in `geothermAt`.

## 7. How to build, run and verify

- Shared build: `mise -C @simulation run build`.
- Private build: `xmake` on the PATH is a mise shim that sets `XMAKE_CONFIGDIR` from `@simulation/mise.toml` (the shared config) whatever you export; call the binary itself: `X=~/.local/share/mise/installs/github-xmake-io-xmake/3.1.2/xmake; cd @simulation && XMAKE_CONFIGDIR=$PWD/../.out/volcanic/xmake $X config --yes --mode=release --builddir=../.out/volcanic/build && XMAKE_CONFIGDIR=$PWD/../.out/volcanic/xmake $X build --yes terra simulation-tests`. Check with `$X show` that `builddir` is `.out/volcanic/build`.
- Own tests: `clang++ -std=c++20 -O2 -Isrc main.cpp test/VolcanicLaws.cpp test/VolcanicLive.cpp ../.out/volcanic/build/macosx/arm64/release/libderivean-simulation.a` with a `main` calling `testVolcanicLaws()` and `testVolcanicLive()`.
- Profiling: the release link strips symbols; link the objects yourself (`clang++ $(find .out/volcanic/build -path "*objs/terra*" -name "*.o") .out/volcanic/build/macosx/arm64/release/libderivean-simulation.a -o .out/volcanic/terra-prof`) and use `sample`.
- Hand-built grounds: `terra volcanic --scene shield|cone|valley|caldera|worn|cinder|summit [--volume km3] [--episodes n] [--quiet steps] [--paroxysm km3] [--rain m/yr] [--slope m/m] --out dir`.
- World: `terra volcanic --survey 1` (planet only, about 40 s); `terra volcanic --x m --z m [--level 1|2] [--tiles n] [--threads n] --out dir --name tag`. A local tile builds its region parents and local neighbours first.
- Read pictures next to the catalogue's photographs (E1–E4).

## 8. Open questions

- Is a 4–13 km volcanic pile under the largest arc centre acceptable? It follows from the planet's supply dealt to one centre and from isostasy sinking every layer.
- Frequency is a factor 5–6 below Earth's on land because of the planet's supply; the lead decides whether the planet's extrusive share or melt production is low.
- Vents under the sea are emplaced like any other (lava quenches to 12 % of its cooling length); not looked at.
- Region against local along X through the arc vent (m, surface, at dx km): region −6 385, −4 929, −2 1511, 0 1336, 2 651, 4 332, 6 85; local (first build) −6 1446, −5.5 2965 (2.8 km loose), −4.5 4131 (3 km loose), −4 1658, −2 1466, 0 1945, 2 762, 4 388, 6 76.
