# Interventions, hydrology, time

Written by the lead from the stream's final report; the stream has finished. Owns `@simulation/src/{hydrology,intervention}`, `test/{Hydrology,Intervention}*.cpp`, `tool/InterventionCommand.cpp`.

## Goal

What the player changes is fed back into the world: dug and built ground, water that fills, spills and escapes, a finite ocean, time that can run, pause and jump — with closed budgets of water, salt and solid.

## State now

`testHydrology` and `testIntervention` pass in a driver linked against the library (the suite stops before them in another stream's check). The game does not use any of this yet.

- Water is one model on two grids: the planet (65 km, periodic) and groups of live Site tiles (4 m). A Site tile becomes live within 256 m of an edit's footprint and stays live; touching live tiles form one group.
- Quiet solver: evaporation and filling solved exactly, then settled (pits fill, full basins overflow, meeting basins merge, salt mixed lake by lake); stable at any step. Event solver: shallow water on eight neighbours with its own stable step while a group is awake after an edit and ticks are at most 4 s.
- Sediment at absolute 5-year marks: only stream power beyond what the untouched bed carried cuts; load drops as a delta in standing water.
- Edits are strokes (lower, raise, level along a segment with width and graded height, or a void opening) kept as a time-ordered history. Jumping backwards rebuilds from the untouched world plus the history.
- Ocean: a void opening drains by orifice flow integrated between cell heights; a sea cut off at a sill keeps its water and salt.
- Three acceptance scenarios run as `terra intervention ocean|cascade|diversion|all [--out dir]` and close water to 1e-11 and solid to 1e-13 (seed 42 at 1.2 Gyr, an earlier planet state): the ocean emptied through the floor in 10 000 years leaving 193 cut-off seas; a breached dam between lakes released 3 120 m³ and after 30 days all three stood as one; a diverted stream moved 27.5 million m³/yr into the new channel and kept it for a century.

## Hooks

- Query, one line after each Site-level `readSurface`: `overlaySurface(world, position, sample)` (`intervention/fn/Intervention.hpp`); exact form `editedTile(world, key)`; per column `siteColumn(world, cellX, cellZ)`. The ground stream wired an overlay as `overlayLive`; check it uses these.
- Sea (`hydrology/fn/Ocean.hpp`): `seaLevelNear(world.ocean, planetPoint(seed, p))`, `seaLevelAt`, `seaDepthAt`, `openWaterAt`, `seaBodies`, `world.ocean.salinity`. Untouched tiles keep `Tile::water` at the pristine sea level; only live tiles follow a falling sea.
- Time: `intervene`, `advance`, `tick`, `jumpTo`, `syncLive`, `budgetOf`; the clock is `world.interventions.calendar`.
- Threading: one thread drives time and edits; readers must not run during those calls.

## Next steps

1. Engine: route the brush into `intervene` strokes (a frame's work as one stroke), drive `tick` and `jumpTo` from the game clock and time controls, re-mesh chunks of changed live tiles, show the ledgers, make the atmosphere read the same sea.
2. Natural events use the same history: the volcanic stream will write eruptions as deposits over time; it needs an additive hook in the time stepping.
3. Infiltration is missing; the groundwater stream supplies the hook for runoff.

## Known weaknesses

A flood that reaches a group's rim leaves it and rivers entering stay at the atlas's flow; jump and walk agree only within tolerance (water 1e-3, salt 2e-2, ground 1e-3, cut volume 5e-2); openings sharing one sea take turns; site-level sea exchange is booked but not fed back to the ocean stock; sediment does not move while a group is in event mode; no halite saturation; no opening under an inland lake; the era cannot change on an existing world. A 100-year jump takes 0.13–0.38 s per tile. The fixes after the second review round were not re-reviewed.

## Requests to other owners (open)

`evaporationDemand` is duplicated in `hydrology/constant/Water.hpp` and should be exported once from `landscape/fn/Forcing.hpp`; escaped water is in `world.ocean.escaped`, not in the planet's ledger.
