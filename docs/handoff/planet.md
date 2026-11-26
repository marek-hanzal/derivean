# Planet, strata, rock mass, ore, heat, spawn

The lead's own stream. Owns `@simulation/src/{planet,strata,rockmass,ore,tectonics,core}`, `simulation/fn/Spawn.*`, `simulation/{type,service}/World.*`, `tool/PlanetCommand.cpp`, `test/{Ore,RockMass}.cpp`.

## Goal

A planet whose history explains what stands on it: where the land is, how high, which rock crops out, how it is broken, where heat rises — as the stage every finer level and every process reads.

## State now

Builds; `testOre`, `testRockMass` and `testPlanetFuture` pass when linked alone against the library (the suite stops earlier in another stream's check). Seed 42 at 4.6 Gyr, from `terra planet`:

- Sea level 54 m, land 21.4 %, mean land height 742 m, highest cell mean 5 366 m.
- Land by height (<200 | <500 | <1000 | <2000 | <3000 | <4000 | above): 38 | 22 | 14 | 15 | 5.2 | 4.6 | 0.6 %. Earth, ice-free: 29 | 28 | 24 | 13 | 3 | 1 | 2.
- Rock at the top of the land: sedimentary 91.9 % (carbonate 10.7), metamorphic 2.7, plutonic 3.2, volcanic 2.2. A valley 150 m deep cuts into: about 68 (7) / 19 / 10 / 3. Earth: 64 (21) / 13 / 7 / 6. These are planet-level estimates; the measure stream's Region-level figures predate the last rock changes.
- Heat: gradient above 60 K/km on 1.5 % of land, above 150 K/km on 0.13 %; veined ground 1.9 %.
- Seeds 7 and 1234 give sea levels of −50 m and −164 m and 20–24 % land.

## What exists

- **Plates travel with their crust** (`planet/fn/Motion.cpp`, `Crossing.cpp`, `Outline.cpp`, `planet/type/Line.hpp`). `Planet::owner` is the live owner of each cell; a cell changes hands only where arriving crust overrides what sinks; new floor at a parting goes to both sides in turn; `Planet::contact` is traced from `owner` every step. `contactAt` in `planet/fn/Sample.hpp` reads it blended.
- **Cover record** (`strata/fn/Strata.cpp`): strata back up with their crust at a collision; `buryCover` turns dated strata under 6 km of younger ones into metamorphic older cover and older cover beyond 6 km into basement.
- **Column** (`strata/fn/Column.cpp`): an unconformity at `StrataColumn::deformed` (strata laid down later share only the gentle warping); plutons cut every older unit as domes (`plutonRoof`); a belt quiet for 800 Myr reads as a shield; `Bed::thickness` is the bed's own thickness before clipping.
- **Solute**: 7 % of eroded rock leaves in solution and precipitates as carbonate on shelves by the room their water leaves, warm ones first; platforms keep 70 m of water (`planet/fn/Surface.cpp`). `ledger.precipitated` is part of `ledger.deposited`.
- **Melt record**: `CrustColumn::melt`, `meltTime`, `erupted`, `meltSource`; `PastMark::melt`, `erupted`; `Planet::meltHeat`. `planet/fn/Heat.hpp`: `geothermAt`, `centreNear` (magmatic centres 46 km apart in the frame of the crust).
- **Rock mass** (`rockmass/`): one structural field for every level — two joint sets in three orders (each 7× wider apart), structural domains 22 km across bounded by faults, attitude of the layering, block size, mass cohesion and friction from the material's `massCohesion` / `massFriction`; `jointDistance`, `fractureShare`, `columnJointDistance`.
- **Ore** (`ore/`): veins around intrusions, pipes in shields; `oreFieldAt`, `cuttingRock`, `nearCuttingRock`.
- **Spawn**: `findVale` (a hospitable low cell under at least 1.5 km of mountains), `findLanding` (the shore in the region with the most country rising over it, then down to a dry site cell beside water), `findShore`.
- **Workers**: `parallelFor` lets the caller work through its own ranges while free threads join in; safe inside a task.

## Tuned constants and what they move

| Constant | File | Value | Effect |
| --- | --- | --- | --- |
| `ArcYield` | `planet/fn/Contacts.cpp` | 1660 | Continental volume: about +1.7 m of sea level per unit; retune after any change to sediment or crust paths |
| `StackLimit`, `SpreadStart`, `SpreadRange` | `Crossing.cpp`, `Contacts.cpp` | 64, 50, 14 km | Height of collision plateaus (cell mean to about 5 km) |
| `Denudation`, `SteepHeight` | `Surface.cpp` | 9 Myr, 2 500 m | How fast high ground is lowered |
| `SoluteShare`, `PlatformDepth` | `Surface.cpp` | 0.07, 70 m | Carbonate share; more solute means more low flat land |
| `ArcSurfaceShare` | `Contacts.cpp` | 0.35 | Lava and ash laid on arcs |
| pluton levels, `RoofDepth`, `RoofMargin` | `strata/fn/Column.cpp` | −0.08 / 0.10, 2 200 m, 0.10 | Plutonic share at the surface |
| `MetamorphicDepth`, `OlderLimit` | `strata/type/Strata.hpp` | 6 km each | Depth to basement |

## Decisions and why

- Fixed mantle-frame plate outlines sliced continents into combs wherever crust crossed a boundary; ownership that travels with the crust removed that. Plates still move by whole cells along the axes and do not rotate, so rifted margins and collision fronts can run straight for a few hundred kilometres.
- Carbonate from a solute pool rather than a facies label: mass stays conserved. A share of 0.18 gave Earth-like carbonate but filled shelves into vast flats (48 % of land below 200 m); 0.07 is the compromise.
- Switched "modes" that stamp landforms were rejected: the owner forbids generated props; the rock-mass field gives the same processes different outcomes instead.
- Every pattern attached to the crust (basement bodies, plutons, structural domains, veins, magmatic centres) is read in ONE frame, `crustFrame` in `strata/type/Column.hpp`: the position minus the travel of the single column that owns it (`crustOwner`, ragged footprint). Blending the travel of neighbouring columns was tried in the heat code and folded the frame wherever crust of different birth meets: a planet cell on an active margin held up to 165 magmatic centres instead of two.
- The planet's melt reaching the surface on land is about half of Earth's per area (44 m per unit of land in 24 Myr against roughly 80 m). If volcanoes should be livelier, raise the supply here (`ArcYield`, `ArcSurfaceShare`, hotspot gain) and retune sea level, not the eruption schedule.
- Changing a planet header needs a full rebuild; a stale object linked against a new `CrustColumn` layout gives silent nonsense.

## Next steps

1. Wire `rockMassAt` consumers: confirm the relief stream uses `fractureShare` to steer drainage; `ore/` should take its vein sets from `rockmass` instead of its own angles.
2. A time-dependent factor on the magmatic term of `geothermAt` near a centre once the volcanic stream provides `ventUnrest`.
3. Land below 200 m is 38 % against Earth's 29 %: look at carbonate platforms that emerge at small sea-level falls.
4. Crust beyond 4.6 Gyr grows without bound (sea level +1 km at 8 Gyr): needs a sink, e.g. sediment taken down with sinking plates in proportion to their cover.
5. Coastline: 37 % of shore cells in axis-aligned runs of five or more (measured before the last changes); plate rotation or sub-cell motion would address the cause.
6. Re-measure rock shares at Region level with `terra measure earth`.

## How to verify

`terra planet --seed 42 --age 4.6e9 --out <dir>` prints the figures above and writes `planet-relief.png`, `planet-plates.png`, `planet-geology.png`. After a planet change check sea level (target within ±100 m of the datum), land share (about 22 %), hypsometry and rock shares on seeds 42, 7 and 1234, then the three tests named above.
