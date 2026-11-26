# Renderer hand-over

Owner of this file: the renderer stream (`@game/src/**` except `map/**`, `@game/main/index.mjs`, `@game/main/capture.mjs`, `@game/index.html`, `@game/capture/**`). It describes the present.

## 1. Goal

The owner's terms:

- "We are supposed to draw a world of cubes, but the render — the LODs — is rounded. Switch everything to cubes or approximations of cubes; the sharp edges will stand out as they should, even in the distance." Far terrain is the voxel world at coarser cube sizes, to the limit of view, drawn by one pipeline with the near voxels. No smooth height meshes, no interpolated normals on terrain.
- "Switch it to WebGPU." three's `WebGPURenderer` (`three/webgpu`) with TSL node materials. Compute is welcome for presentation, forbidden for anything that decides the world.
- Still binding: no heat-map look; colour only from the simulation's materials; view to at least 250 km with aerial perspective; frame time in boost flight must hold.

Sequencing: the WebGPU path is built beside the WebGL one and chosen by `DERIVEAN_RENDERER=webgpu`. Flip in one step once terrain, water, sky, air and exposure run on WebGPU, deleting the WebGL path in the same change. Clouds, cloud shadows and precipitation may follow the flip.

## 2. State now

Works on the WebGPU path (verified on macOS, Apple GPU, Electron 45.5, three 0.187, by replaying recorded engine frames through the built app; `backend` reads `WebGPU`, no page warnings or errors; `tsc --noEmit` passes):

- Voxel chunks and far cube levels 2 to 13 from `Frame.chunks[].quads` and `Frame.cubes`, one material, one draw per cube size (39 draws for a ground view holding 7.3 M triangles; draw call CPU 0.5 ms; frame p50 8.4 ms at machine load about 25).
- Level clip from `Frame.cubes.levels[].box` and `Frame.voxelBox`, flat normals, corner occlusion, per-cube tone, world-anchored ground variation.
- Standing water sheets from `CubeChunk.water`, shore depth from a copy of the scene depth.
- Sky background, aerial perspective, exposure, plain shading (L).
- F3 and every shot record say the backend and the draw's CPU and GPU milliseconds.

Not verified:

- A run of the real app with the live engine under `DERIVEAN_RENDERER=webgpu`. Only replays have been drawn.
- Detached pieces, voxel water, particles and the brush on WebGPU: written, no recording holds any.
- The GPU milliseconds: three's timestamp query reads 17 to 26 ms on views whose frames take 8.4 ms, so the number is not trusted yet.
- Linux, Windows, and three's WebGL 2 fallback backend.

Missing on the WebGPU path: sun shadows, clouds, cloud shadows, precipitation, snow on cube tops, culling inside a batch.

Known faults in the picture: shaded risers are very dark against sunlit tops (zebra bands on slopes seen against the sun); the sun's glint on water is too large; small white dots along some step edges at 300 m, cause not found yet.

The WebGL path is unchanged and still the default.

## 3. In progress

Judging the first land captures in `.out/playwright/cubes-wip/` (`land-*.png`) and tuning light: ground bounce in `sky/service/Daylight.ts`, occlusion strength `Open` in `cubes/fn/cubeSurface.ts`. No file is half-edited; the tree builds.

## 4. Next steps

1. Find the white dots (crop `land-300m-n.png` around x 1000 to 1300, y 520 to 640). Suspects: the dithered band of a finer level over the coarser one, or back faces showing.
2. Lift shaded faces: make `groundLight` the bounce of sun and sky off the ground instead of the old fixed brown; shrink the water glint.
3. Run the real app: `DERIVEAN_RENDERER=webgpu DERIVEAN_CAPTURE=<plan> @game/node_modules/.bin/electron @game` with a plan that pins `world.spawn` to land, and the same plan without the variable for the smooth half of the capture pair. Put the pair in `.out/playwright/look/`.
4. Measure boost flight on both paths with `boost-30s-clear`; state the machine load next to every number.
5. Sun shadows from the cube batches: two orthographic depth passes (near, 120 km) drawn with a depth-only material that shares `faceNodes().corner`; sample in `cubeSurface`.
6. Snow on tops from `CloudField.snowLadder` and `snowLine` (rule in section 6), evaluated per vertex.
7. Independent review, then the flip: make WebGPU the default, delete the WebGL path (list in section 5), drop `WorldOptions.cubes` handling in `session/fn/cubeWorlds.ts` once the engine makes cubes only.
8. After the flip: clouds, cloud shadow, precipitation, particles as node materials, trusted GPU timings.

## 5. Decisions and why

- One batch per cube size (`cubes/service/FaceBatch.ts`), not one mesh per chunk. Per-chunk meshes cost 10 ms of CPU a frame for 1150 draws in three's WebGPU renderer; batches cost 0.5 ms. The price is no frustum culling inside a batch.
- GPU face layout is four `uint32` words with sixteen-bit positions counted from the batch's anchor (`cubes/fn/packFaces.ts`), not the engine's two words. A face then needs no per-chunk uniform or lookup, and debris and water lists, whose positions do not fit six bits, use the same layout.
- The level is carried in each face's bits and the level boxes in one uniform array (`cubes/type/LevelTable.ts`), so every level shares one material.
- Level boundary: the finer level thins out by dither over three quarters of its band beyond its box; the coarser one is cut hard at the finer box. Faces are drawn double-sided and back faces are lit as dim ground: where the coarse level stands higher at the cut, its inside closes the step instead of opening onto the sky. This is a labelled trick. Sinking the coarse level by vertex displacement was rejected: greedy quads crack at T-junctions.
- A face on a box side goes with its cube: the clip tests a point a hundredth of a cube inside the face.
- Until clouds are drawn on WebGPU the light is the clear sky's (`Daylight.cover` is not called), so sky and ground agree.
- Octaves of ground variation finer than the cube they are drawn on are left out; they would be noise from cube to cube.
- Deep water is opaque (visibility about 25 m); the old 20 % see-through showed a sunlit bed 2 km down.
- No shore line is drawn: the `fwidth` line of the smooth water fires on every step of a cube bed.

Traps:

- `DynamicDrawUsage` on an attribute makes three's WebGPU renderer upload it every frame (56 MB a frame here). Leave attributes static; `needsUpdate` with `addUpdateRange` still works.
- `viewportDepthTexture()` copies into a `depth24plus` texture and fails against the float depth buffer that `reversedDepthBuffer` uses. Pass a `DepthTexture(1, 1, FloatType)`: `new ViewportDepthTextureNode(screenUV, null, depth)`.
- A `color` uniform is typed to multiply by floats only; write `vector.mul(colour)`, not `colour.mul(vector)`.
- TSL `mod` is avoided; `groundTone.ts` wraps by hand so negatives wrap as in GLSL.
- Every stream builds into and captures from the one `@game/dist`. Keep the tree building at all times.
- The contract moves: on 26 Nov it changed from nine `int16` a face to `Quads` during this work. Read `GameBridge.ts` before trusting a recording or this file.
- The engine's landing and the planet's surface move between engine builds. Pin `world.spawn` or a shot `position` for like-for-like captures.

Deleted at the flip (WebGL path): `scene/service/WorldScene.ts`, `WorldStage.ts`, `VoxelTerrain.ts`, `VoxelOcclusion.ts`; `scene/fn/voxelSurface.ts`, `voxelGeometry.ts`, `meanNormals.ts`, `looseness.ts`, `surfaceTone.ts` (keep `renderOriginStep`); `scene/constant/ground.ts`; `scene/type/*`; `terrain/**`; `sky/service/Sky.ts`, `sky/fn/dome.ts`, `sky/constant/skyRadiance.ts`, `airFade.ts`, `shadow.ts`, `sky/type/Air.ts`; `water/fn/naturalWater*.ts`, `water/service/NaturalWater.ts`, `WaterScene.ts`; `diagnostics/service/GpuTimer.ts`; and `clouds/**`, `weather/**` rendering once ported. `map/**` draws through its own WebGL 2 canvas (`map/service/ChartSurface.ts`) and is not touched by the switch.

## 6. Contracts with other streams

Engine (`@game/contract/GameBridge.ts`, read it for the truth):

- `WorldOptions.cubes: true` starts a world of cubes; the WebGPU path adds it in `session/fn/cubeWorlds.ts`.
- `Quads`: two `uint32` a quad, chunks 32 cubes a side. Word 0: plane (bits 0 to 5), lowU (6 to 11), lowV (12 to 17), sizeU − 1 (18 to 22), sizeV − 1 (23 to 27), axis (28 to 29), positive normal (30). Word 1: material (0 to 7), corner occlusion (8 to 15, two bits a corner from (lowU, lowV), (highU, lowV), (lowU, highV), (highU, highV)).
- `ChunkMesh.quads` is the ground of a voxel chunk; `ChunkMesh.faces` (`WorldInfo.faceWords` shorts a face) stays for water meshes and debris.
- `CubeChunk { id, level, edge, origin, quads, water: { level, quads }[] }`; sheet quads have plane 0 and are drawn at `level` metres.
- `Frame.cubes.levels[]`: `{ level, edge, wanted, shown, box }`. A level is drawn inside its box and outside the next finer box; the finest outside `voxelBox`, whose footprint counts at any height.
- Snow, not drawn yet: `CloudField.snowLadder` (kg/m², rungs at `WorldInfo.snowLadder` metres above `seaLevel`), `snowLine` per cell. Nothing below the snow line, linear from there to the first rung above and between rungs, end rungs held, bilinear between four cells. Tops only.

Asked of the engine stream, not yet answered:

- One chunk can carry two sheets at the same water level (seen at 35.5 m over the sea), which blend twice.
- Views hold 6.5 M quads (levels 7 and 8 hold about 1 M each at a land spot); a way to cull, or fewer hidden faces, would help.
- `Frame.pending.patches` and `ready` are what a capture waits on; confirm they cover cube chunks.

Lines added to shared files:

- `@game/src/index.ts`: imports of `createCubeScene` and `cubeWorlds`; in `compose()` the `cubes` constant read from `location.search` and the `scene` choice; `cubeWorlds(window.derivean)` passed to the session; `backend: scene.backend, draw: scene.timings()` in `metrics.paint`; `backend: scene.backend, drawMs: scene.timings()` in `measure`.
- `@game/src/capture/service/CaptureDriver.ts`: `|| frame.cubes?.added.length` in the arrival test, so a cube world is not taken as stalled while it streams.
- `@game/main/index.mjs`: `DERIVEAN_RENDERER=webgpu` sets `renderer.search`; `loadURL(renderer.href)` replaces `loadFile`, and the sender check compares against the same address.

The M key and `map/**` are the map stream's. The window title stays "DeRivean · Fable".

## 7. How to build, run and verify

- Build: `npm run build` in `@game` (Vite; it does not type-check). Types: `npx tsc --noEmit` in `@game`.
- Real app: from the worktree root, `DERIVEAN_CAPTURE=<plan.json> @game/node_modules/.bin/electron @game`; add `DERIVEAN_RENDERER=webgpu` for the cube path. It uses `.out/addon/derivean.node` as built by the engine stream. Engine start takes 2 to 8 minutes under load. Plans: `@game/capture/look.json` (37 shots). Shots and records land in the plan's `out`.
- Replay, the fast loop (about a minute, no engine): tooling in `.out/renderer-replay/` (scratch, untracked). Record once with `ELECTRON_RUN_AS_NODE=1 @game/node_modules/.bin/electron .out/renderer-replay/record.cjs <worktree> <outDir> 1 <spawnX> <spawnZ>` (ground, 300 m and 2 km above a spot; about 7 minutes), then `REPLAY_DATA=<outDir> .out/renderer-replay/run.sh .out/renderer-replay/plan-land.json`; edit the paths at the top of `run.sh` first. It runs the built app against the recorded frames through the real preload and capture driver and prints draws, triangles, backend and timings per shot. A recording is stale as soon as the contract changes. Land spot used: spawn `[-2521902, -4671906]` (a lake at 735 m among hills, seed 42, 4.6 Gyr).
- Frame statistics are not trustworthy above load about 40; say the load with every number. Close every instance you start; other streams run instances too, so check a process's parent and working directory before ending it.
- Good looks like: the same view in cubes at every distance; walls, ledges and crests crisp; strata as bands on far cliffs; cubes told apart by light and corner occlusion under overcast; no shimmer where risers are near a pixel; no holes at level boundaries; aerial perspective to the limit of view; water as flat sheets with a shore; snow above a snow line. Compare with `docs/reference-landscapes.md`.
- WebGL baseline for comparison: `.out/playwright/look/` (smooth patches; boost flight p50 8.3 ms, p95 16.6 ms, 560 to 630 draws, about 1 M triangles, cloud pass 6 to 17 ms GPU).

## 8. Open questions

- Is the GPU within budget? Frames hold 8.4 ms in replay while the timestamp query says 17 to 26 ms. Needs a trusted measure and boost flight on the live engine.
- Whether 6.5 M unculled quads hold frame time on weaker GPUs; splitting a batch by quadrant or indirect multi-draw are the candidates.
- Risers of a one-cube step show the bed's material under a covered top, which stripes gentle slopes. Is that the wanted reading of "strata as bands", or should the top cube's sides carry the cover?
- Whether the dithered band is enough against popping when a level's box moves, or the band should be wider than four cubes.
