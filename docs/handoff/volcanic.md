# Hand-over: volcanic

Describes the present state of the `volcanic` workstream. Rewrite it when the state changes; it is not a diary.

## 1. Goal

Own `@simulation/src/volcanic/**` (+ `test/Volcanic*.cpp`, `tool/Volcanic*.cpp`): volcanic landforms that arise from simulated emplacement at the planet's magmatic centres during a region's replay, are carried down the levels as rock, and keep living in calendar time during play. Checked against `docs/reference-landscapes.md` E1–E4: stratovolcanoes 1–3 km over a 10–30 km base with concave flanks, shields of a few degrees, calderas 2–20 km across, lava fields and plateaus, cinder cones in groups, necks. The volume built must be what the planet's record says reached the surface. Nothing placed, nothing painted by noise.

Order of work given by the lead: replay edifices → vent state, schedule and inspector data → live emplacement through the session overlay and the time jump → atmosphere coupling.

## 2. State now

Compiles with the rest of the tree. Run on world tiles at the arc of seed 42 (the largest andesite vent, `--x 13686236 --z 11027538`, an island volcano by the coast). Tests `testVolcanicLaws`, `testVolcanicLive`, `testHydrology` and `testIntervention` (now with `testEruption`) pass through private drivers (the suite stops earlier, at another stream's test).

- **Live deposits** (new): the interventions module has an `EditKind::Deposit` edit (section 6) that lays an eruption's near field into the live site columns, kept in the history and replayed by `jumpTo`; `depositOf` turns `layOutbreak`'s result into one. `testEruption` lays a small andesite eruption beside the stream of `testIntervention`, walks a month, jumps back before it (nothing left) and forward again: the deposit comes back exactly, the ground and water a month on within 1e-3, the column over the vent within 5 cm. testIntervention takes 279 s at load 35.
- **Region tile**: 13–15 s for one tile (`.out/volcanic/world/arc-region.txt`, `volcanic-arc-region-map.png`).
- **Local tile**: 259 s for the requested tile including the planet, the region parents and the five local tiles it waits for, on a machine at load 40 (`.out/volcanic/world/arc-local-trace.txt`, map `volcanic-arc-local-map.png`); the emplacements of the local level took 72 s of that, the slowest 1.9 s. The predecessor's run that never finished had two causes: (1) "minor" eruptions were capped in number (8 per episode) but not in size, so the largest vent built 11 km³ "minor" eruptions at 64 m cells (50 s each); now each keeps its typical size and the region keeps the rest; (2) fall landing on ground steeper than its angle of rest relaxed to a few millimetres (`Relax.cpp` slack), up to 30 s per cone; the slack is now also 0.5 % of the fall a layer holds over one cell (10–25× faster, region forms unchanged within a few metres).
- **Region against local at the vent** (scratchpad drivers `levels.cpp`, `towers.cpp`): the local surface follows the region's within about 300 m along a line through the vent. A first build showed two cells 4.5–5.5 km west of the vent under 2.8–3 km of loose cover; a later build of the same tile (with other streams' changes in the tree meanwhile) has none: deepest loose cover 116 m, at the sea edge 7.8 km from any minor vent, mean 1.2 m. Not attributed; watch for it.
- **The pile under the arc vent is 4–13 km thick** at the region (datum 3.5–11 km below sea level). It is the standing pile (rock above its floor after erosion), not the emplaced total, but the two differ little: the centre was dealt 17 600 km³ over 22.4 Myr (40 awake steps) by a conduit that wanders within 6 km; about 60 % of it lands within 20 km (lava, fall, currents), some 8 km on average; every layer then sinks by its own thickness (the planet already raised the province for all its melt; spread over 28 km, `fn/Settle.cpp`), so the pile goes down below the datum and erosion only reaches its top. Real arc volcanic sections are 1–5 km; the cause is one centre fed for 22 Myr in one place (section 8, proposal to the lead).
- **Sealing removed** (lead's decision): fresh volcanic rock erodes as its rock does. Scenes (`.out/volcanic/scene2/`): cone 697 m over its foot after 6 × 60 km³ (990 m with sealing), shield 474 m, worn cone gone after 10 Myr; caldera pit 114–118 m after the step that founders it (test threshold lowered from 200 to 100 m: the steep wall is the local level's). Local scenes: `summit` builds an 847 m andesite cone at 33–35° with a lava apron, `cinder` a 274 m basalt cone at 31–35°.
- **Voxel side** (predecessor's wiring, finished and reviewed here): datum clamped to `rock − pile` in `readSurface` (now with the pile interpolated by the same weights as the cover) and `GroundLattice::at` (pile kinds from the nearest node that has a pile, so margins read as their pile and not as tuff); `formColumn` reads the pile's members above its floor with their own faces and partings (recess by the member's rock), plugs cut pile and layering; the ground level reads pile beds through `volcanicUnit` (also dates the rock mass now); `ground/fn/Partings.cpp` needed no change.
- **Live eruptions, simulation side** (new, `volcanic/type/Outbreak.hpp`, `volcanic/fn/Outbreak.{hpp,cpp}`): `outbreaksBetween` gives a vent's own eruptions (the replay's sequence) in any window of era years, identical however the window is cut; `outbreakOf` gives site, style split, rates, durations, column (Mastin 2009) and lava plume (Settle 1978); `airSourcesOf` turns one into `AirSource`s; `layOutbreak` lays the near field (cone, currents, lava, caldera drop) on any `Emplacement`. Eruptions of the local level's size use its laws (short flows, cones of fragments). Figures: basalt 0.1 km³ → a day of fountaining to 4 km and 23 days of lava; andesite 0.1 km³ → 29 h column 9 km, then a dome for a year; 5 km³ andesite → 20 h at 26 km; 30 km³ → caldera, 16 h at 48 km. Nothing calls these yet (engine) and the terrain change needs an intervention edit kind (section 6).
- **Frequency and size** (survey, seed 42, `.out/volcanic/world/survey.txt`): 13.5 eruptions a year on land (4.1 before the chamber law), against Earth's 20–28 for this much land (50–70 a year on 2.47 times our land). By VEI (bulk tephra = 2.5 × the fragments of `outbreakOf`), ours a year against Earth's recurrence (Newhall and Self 1982) scaled to our land: VEI 0–1 9.9 (Earth's VEI 0–1, the rest of its 50–70, about 10–18); VEI 2 2.5 / 8.9 (0.28); VEI 3 0.87 / 1.35 (0.64); VEI 4 0.21 / 0.24 (0.87); VEI 5 0.015 / 0.028 (0.54); VEI 6 0.0001 / 0.005; VEI 7 none / 0.0005. Before the chamber law the small tail was missing (VEI 0–1 1.3, VEI 2 0.19): every vent drew around one median, so large centres made too many small eruptions and small centres too few. Volume reaching the surface on land now is 0.081 km³/yr (0.11 in the window's mean); Earth's subaerial output for as much land is roughly 0.2–0.37 km³/yr (arcs about 0.5 km³/yr and subaerial hotspots and rifts about 0.2 worldwide, Crisp 1984, from memory): the planet's supply is short by about 3 (2.5–4.5), most visibly in effusive basalt on land (429 basalt vents of 2 491; VEI 0–1 hold 2.5 % of our volume). One supplied vent per 42 000 km² of land, neighbours 17–55 km apart; from a random place on land the nearest supplied vent is 65 km (p10) to 2 100 km (p90) away.

## 3. In progress

Nothing half-done.

## 4. Next steps

1. A 2 × 2 local run at the arc for seams and a look at `terra view` of the local edifice; watch for the kilometre columns of loose cover seen once (section 2).
2. Engine wiring of live eruptions (section 6, the engine's); unrest in the inspector (`volcanicAt`). Live lava shows in voxels as the host rock until the live columns carry the pile (section 6).
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
- **Schedule**: steps of a vent are awake in spells of four region steps (70 % awake) with log-normal unevenness 1.2; the conduit moves every 2–7 steps within 6 km (andesite) or 10 km (basalt) of the centre. Single eruptions are a seeded sequence of the vent (`fn/Chamber.cpp`): the chamber fails when it holds the next one's volume, so the replay and the calendar share one history.
- **Eruption size is the chamber's** (`fn/Chamber.cpp`): a vent's chamber stores 1e4 yr of its window-mean supply (`Vent::chamber`); an eruption lets out chamber × overpressure (5 MPa) × compressibility (basalt 2e-10 /Pa, gas-rich andesite 1e-9 /Pa) in the median, 0.1 % and 0.5 % of the chamber, log-normal with spreads 1.8 and 2.2, and never more than the chamber. A typical centre (0.06 km³/kyr) holds 0.6 km³ and erupts 0.6e6 m³ (basalt) or 3e6 m³ (andesite) in the median; the largest arc centre holds 7.3 km³. This is what raised the count from 4.1 to 13.5 a year with the same supply; the inputs are physical (Huppert and Woods 2002; residence of eruptible melt 1e3–1e5 yr) and not fitted to Earth's count.
- **Caldera**: a single eruption that takes 30 % of its chamber and at least 1 km³ (basalt) or 2 km³ (andesite) founders a block 500 m or 1 200 m deep (`emptiesChamber`), as wide as its volume requires, at the END of its step. Pinatubo-sized eruptions (5 km³) now empty a 5 km³ chamber, as Pinatubo did. At 1 km cells the region's slope failure (which lets a kilometre-wide cell stand at 0.45 of a rock face) turns the ring wall into a 15° funnel. Planned and not built: the local level re-draws the wall as narrow as its own cells allow by applying `founder(local) − spline(founder(region))`, only where the pile's top is not younger than the collapse.
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
- `test/Intervention.cpp`: `testEruption(world, site);` after `testDiversion`; `test/InterventionShared.hpp`: its declaration (both now this stream's, with the interventions module).

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

**Live deposits** (`intervention/type/Edit.hpp`, `intervention/fn/Raster.cpp`, owned here since the interventions stream closed):

```cpp
struct Deposit { Vec2 origin; double cell; int width, height; std::vector<float> drop, rock, loose[SedimentClasses]; };   // m per cell, row-major
Edit deposit(std::shared_ptr<const Deposit> laid);                                     // EditKind::Deposit over the grid's rectangle
std::shared_ptr<const Deposit> depositOf(const Emplacement& field, const NearField& near);   // volcanic/fn/Outbreak.hpp
```

Each site cell takes the area mean of the grid over its square, so any cell size works and the volume is kept where the grid lies on live tiles (the edit makes every tile within 256 m of its rectangle live). Per cell: the floor founders by `drop` with its cover, `rock` is laid on it and seals the cover it buries into rock, `loose` lies on top by class; water keeps its depth on the new ground. Booked as `dug` (drop) and `filled` (the rest), so the solid budget closes. `depositOf` uses the replay's rule (`WeldDepth`, now in `constant/Rheology.hpp`): 2 m or more in a cell is rock, thinner lies loose (lava as coarse rubble, fragments half coarse, half sand). The history holds the deposit itself, not a recipe, so a jump reproduces it exactly whatever the ground looked like; what water does with it afterwards agrees within 1e-3 (as for strokes).

The engine's sequence (to wire):
1. Ahead of the calendar (say a year), `outbreaksBetween(seed, vent, eraYear(now), eraYear(now + window), list)` for the vents of the region tiles around the player (`Tile::volcanic->vents`); `eraYear = world age + calendar seconds / YearSeconds`.
2. For each, `airSourcesOf(outbreak, calendar.era, groundAtVent, sources)` and `addSource` each before time passes its start.
3. When the calendar passes the end of its phases (`time` + `blast` + `effusion`): an `Emplacement` around `outbreak.site` with its origin on the lattice of its cell (4 m for small eruptions; 16–64 m for a flow of a cubic kilometre, which covers tens of km²), `ground` and `water` from `surveyAt`, `rest` Open where the engine allows change and Closed beyond; `near = layOutbreak(field, outbreak)`; `intervene(world, deposit(depositOf(field, near)))`. Lay it once; do not lay it again after a jump: the history has it.
4. Wind-borne ash: `ashFallAt` (kg/m²) / bulk density (about 1 000 kg/m³) as `loose[Fines]` (and `Sand` for grit) of a Deposit on the engine's own grid, when the fall has ended.

Costs: every site tile within reach of the rectangle becomes live and stays live (one km² per tile, its water followed in time), so a basalt flow of 0.1 km³ makes some 30 tiles live. Not done: the live columns know rock and cover only, so voxels draw new lava as the host rock (`changeLattice` moves rock without kind). Needed from `simulation/fn/Live.cpp`'s owner: an `EditedTile` array of volcanic rock laid (m) and its kind, read by `changeLattice` into `GroundLattice::pile`.

**Later**: `ventUnrest` for the lead's time-dependent term in `geothermAt`.

## 7. How to build, run and verify

- Shared build: `mise -C @simulation run build`.
- Private build: `xmake` on the PATH is a mise shim that sets `XMAKE_CONFIGDIR` from `@simulation/mise.toml` (the shared config) whatever you export; call the binary itself: `X=~/.local/share/mise/installs/github-xmake-io-xmake/3.1.2/xmake; cd @simulation && XMAKE_CONFIGDIR=$PWD/../.out/volcanic/xmake $X config --yes --mode=release --builddir=../.out/volcanic/build && XMAKE_CONFIGDIR=$PWD/../.out/volcanic/xmake $X build --yes terra simulation-tests`. Check with `$X show` that `builddir` is `.out/volcanic/build`.
- Own tests: `clang++ -std=c++20 -O2 -Isrc main.cpp test/VolcanicLaws.cpp test/VolcanicLive.cpp ../.out/volcanic/build/macosx/arm64/release/libderivean-simulation.a` with a `main` calling `testVolcanicLaws()` and `testVolcanicLive()`.
- Interventions: the same with `test/Intervention.cpp test/InterventionReplay.cpp test/InterventionEruption.cpp test/Hydrology.cpp test/HydrologyGround.cpp test/HydrologySea.cpp` and a `main` calling `testHydrology()` and `testIntervention()` (about 5 min under load).
- Survey prints the VEI table (`tool/VolcanicSurvey.cpp`, `sizeFrequency`).
- Profiling: the release link strips symbols; link the objects yourself (`clang++ $(find .out/volcanic/build -path "*objs/terra*" -name "*.o") .out/volcanic/build/macosx/arm64/release/libderivean-simulation.a -o .out/volcanic/terra-prof`) and use `sample`.
- Hand-built grounds: `terra volcanic --scene shield|cone|valley|caldera|worn|cinder|summit [--volume km3] [--episodes n] [--quiet steps] [--paroxysm km3] [--rain m/yr] [--slope m/m] --out dir`.
- World: `terra volcanic --survey 1` (planet only, about 40 s); `terra volcanic --x m --z m [--level 1|2] [--tiles n] [--threads n] --out dir --name tag`. A local tile builds its region parents and local neighbours first.
- Read pictures next to the catalogue's photographs (E1–E4).

## 8. Open questions

- **Pile 4–13 km** (to the lead): what should bound it is the life of a magmatic centre. Stratovolcanoes and their clusters live 0.1–2 Myr and the focus of an arc segment moves by tens of km between them; here `centreNear` (planet/fn/Heat.cpp) is a fixed jittered lattice in crust coordinates, so one centre is fed for the whole 24 Myr window. Proposal for Heat.cpp: `constexpr double CentreLife = 1.5 * Myr;` and `centreSite(planet, material, epoch)` drawing the lattice from `stream(planet.seed, CentreStream) ^ uint64_t(epoch)` with `epoch = floor(time / CentreLife)`, `centreNear(planet, position, time)` passing it (vigour drawn per epoch too); the volcanic side would then list the centres of every epoch the window spans (`fn/Centres.cpp`) and schedule each within its epoch (`fn/Source.cpp`). A centre would get some 1 100 km³ instead of 17 600 (a large stratovolcano complex) and the pile under any point would fall several-fold. Secondary bounds: the sink spread (28 km) is narrower than an arc plate's flexure (Te 10–30 km bends over 50–150 km), which buries each layer under its own load instead of letting it stand to be eroded, but the halo does not allow wider; spreading of the edifice is already held by the flow law (flanks 9–26°, 2.7 km over its foot); erosion acts on the top since sealing went.
- **Frequency**: the planet's supply on land is short by about 3 (lead's). Of the size distribution, mine and open: VEI 2 is short by 3.6 (small andesite eruptions come out as domes or lava, VEI 1, where Earth's are vulcanian blasts) and VEI 6–7 are missing (no chamber above 7 km³: Earth's come from silicic reservoirs that store melt 1e5 yr, a magma this model does not have).
- Vents under the sea are emplaced like any other (lava quenches to 12 % of its cooling length); not looked at.
- Region against local along X through the arc vent (m, surface, at dx km): region −6 385, −4 929, −2 1511, 0 1336, 2 651, 4 332, 6 85; local (first build) −6 1446, −5.5 2965 (2.8 km loose), −4.5 4131 (3 km loose), −4 1658, −2 1466, 0 1945, 2 762, 4 388, 6 76.
