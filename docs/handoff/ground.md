# Ground level and voxel forms

Written by the lead from the stream's final report; the stream has finished and has no active owner. Owns `@simulation/src/{ground,relief}`, `simulation/fn/{Query,Reader,Live}.*`, `tool/{Ground*,Slice,Block}Command.cpp`, `test/Ground*.cpp`, and [simulation.md](../simulation.md).

## Goal

Ground a person stands on that is not empty and not smooth: stones, rills, ledges, step-pools, frost forms and spring deposits that arise from simulated processes at 0.25 m, and a voxel answer that shows them.

## State now

`test/GroundForms.cpp` and `test/GroundTile.cpp` pass when linked alone against the library (0.6 s and 11.5 s). The suite and the root check fail elsewhere (see [README](README.md)).

- `Level::Ground`: 0.25 m cells, 64 m tiles, 24 steps of 1 000 yr, same halo and four-pass scheme; `replayTile` is the one builder and calls hooks in `ground/fn/Stages.hpp`.
- Rock at this scale: layers and weak seams inside each bed (`ground/fn/Partings.cpp`); joints from the shared `rockmass` field; ore veins count as rock of their own; slots, towers and arches in `relief/fn/City.cpp` read the same planes.
- Stones are individuals with size, shape, attitude, rock, source position and travel, each paid for out of the coarse layer. Fallen blocks are followed one by one over the Site surface from cells that shed rock (`Tile::shed`), up to 124 m.
- After settling (`ground/fn/Finish.cpp`): frozen-ground flow and sorting, stones, stream-bed steps and pools, spring deposits, then water.
- Queries: `sampleBlock` reads ground tiles plus seams, ore, caves, spring crust and stones; `surfaceAt` / `surveyAt` take a `finest` level (default Site); `stoneAt`; `overlayLive` lays session edits over answers by site cell (`simulation/fn/Live.cpp`).
- Measured on seed 42 (planet state before the last rock changes, machine loaded): 1 907 stones per hectare on a granite headwater, 668 on a limestone plain; rill spacing median 8–12 m with scatter 0.8–1.0; coarse budget closes exactly; one tile 1.2–1.7 s on one thread; a 4 m voxel chunk 0.56–0.74 ms; 100 m × 100 m of new ground in 5.5 s with Site tiles present.

## What arises from what

Evolved in the settle: rills, ledges over strong layers, knobs between joints, patchy mantle, sorted nets and stripes, spring terraces and cones. Evolved per block: fallen blocks (size and deflection are seeded draws). Seeded packing of the simulated coarse layer: lag, scree, stream armour, till boulders, frost blocks. Constructed once from stone positions and flow: steps, ribs, pools. The table in [simulation.md](../simulation.md) is the full one.

## Open items

- A temporary file `ground/fn/GroundwaterStandIn.cpp` defines `springsIn`, `caveFieldAt`, `caveSpans`, `inCave` as empty; the groundwater stream deletes it when its definitions link. Hydrothermal ground and caves were tested only with synthetic springs and this stand-in.
- Not seen at a real place on the final build: till and erratics, block fields, patterned ground, a lowland river, a glaciated valley, a talus foot.
- Water levels do not always meet across tile edges; two stones can overlap across a tile edge; the determinism test covers a second-pass tile, not a fourth-pass one.
- Absent: kettle holes, landslides, earth pillars, potholes, ripples, limestone grikes and rillen (limestone is now common on the planet).
- Late edits (overlay, pool use, rock-cut share, `stoneAt`) were not re-reviewed.
- Pool gain inside a tile is 5–17 %: flooding, routing, cutting and failure are ordered work.
- A ground tile holds about 7 MB and the atlas never evicts.

## Contracts other streams add here

Other streams wire their volumetric results into the voxel answer by exact-match edits next to the cave spans: the coast stream's `seaVoidsOver` / `seaVoidSpans` (`coast/type/SeaVoid.hpp`), the volcanic stream's `pileColumn` / `pileBedAt` / `neckOf` (`volcanic/type/Pile.hpp`, `volcanic/fn/Volcanic.hpp`), the groundwater stream's karst slots. Each must extend the alignment test: `materialAt` equals what `sampleBlock` returns for any block size and alignment.

## For the engine

Material ids Sinter 37 and Travertine 38; gate chunks on `sim::BlockGroundReach` (7.5 m) and `sim::BlockSiteReach` (40.5 m); stones stand up to 3.7 m over the ground surface (`FormRise` of 1.5 m is too small); readers that call `readSurface` must call `overlayLive`; `sampleBlock` on background threads needs a gate against `intervene` / `advance` / `tick` / `jumpTo` / `syncLive`.

## How to verify

`terra ground --find 1 --out <dir>` finds a place and draws plan and section; `terra slice --x --z --span --step --tall --out <dir>` shows the voxel answer. Pictures the stream judged are under `.out/terra/ground/` (not in Git).
