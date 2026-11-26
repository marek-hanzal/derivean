# Renderer hand-over

Owner of this file: the renderer stream (`@game/src/**` except `map/**`, `@game/main/index.mjs`, `@game/main/capture.mjs`, `@game/index.html`, `@game/capture/**`). It describes the present.

## 1. Goal

The owner's terms:

- "We are supposed to draw a world of cubes, but the render — the LODs — is rounded. Switch everything to cubes or approximations of cubes; the sharp edges will stand out as they should, even in the distance." Far terrain is the voxel world at coarser cube sizes, to the limit of view, drawn by one pipeline with the near voxels. No smooth height meshes, no interpolated normals on terrain.
- "Switch it to WebGPU." three's `WebGPURenderer` (`three/webgpu`) with TSL node materials. Compute is welcome for presentation, forbidden for anything that decides the world.
- Still binding: no heat-map look; colour only from the simulation's materials; view to at least 250 km with aerial perspective; frame time in boost flight must hold.

Sequencing: the WebGPU path is built beside the WebGL one and chosen by `DERIVEAN_RENDERER=webgpu`. Flip in one step once terrain, water, sky, air and exposure run on WebGPU, deleting the WebGL path in the same change. Clouds, cloud shadows and precipitation may follow the flip.

## 2. State now

Works on the WebGPU path. Verified on macOS, Apple GPU, Electron 45.5, three 0.187, by a run of the real app on the live engine (`@game/capture/pair-cubes.json`, ten shots, `backend` reads `WebGPU`, no page warnings or errors) and by replays of recorded engine frames; `tsc --noEmit` passes:

- Voxel chunks and far cube levels 2 to 13 from `Frame.chunks[].quads` and `Frame.cubes`, one material, one draw per cube size: 34 to 39 draws for views holding 2.6 to 6.2 M triangles.
- Level squares from `Frame.cubes.levels[].box` and `Frame.voxelBox`, cut on the coarser level's lattice and closed by cap risers, so no hole opens between levels. Flat normals, corner occlusion, per-cube tone, world-anchored ground variation.
- Standing water sheets from `CubeChunk.water`, shore depth from a copy of the scene depth.
- Sky background, aerial perspective, exposure, plain shading (L).
- F3 and every shot record say the backend and the draw's CPU and GPU milliseconds.
- The world is not drawn while the map is open (`scene.covered = map.open` in `index.ts`, both paths); simulation and streaming go on.

Measured on the same plan, spot and engine build, machine load 60 to 140 (far above the 40 at which frame statistics can be trusted, so read these as "holds", not as a ranking):

| | WebGL, patches (`pair-smooth`) | WebGPU, cubes (`pair-cubes`) |
| --- | --- | --- |
| Static shots, frame ms p50 / p95 / p99 / max | 8.3–9.4 / 9.6–17.5 / 10.3–22.9 / 10.4–49.9 | 8.3 / 9.1–10.1 / 9.9–10.4 / 10.3–10.4 |
| Boost flight 30 s at 382 m/s, clouds off | 8.4 / 16.9 / 21.0 / 28.1 | 8.3 / 9.8 / 16.7 / 25.0 |
| Draw calls | 391–1252 | 34–39 |
| Triangles | 0.75–2.3 M | 2.6–6.9 M |
| GPU ms | clouds 9–16 (the only pass timed) | whole frame 4.2–6.7, draw call CPU 0.3–0.4 |

The WebGL shots other than `300m-north-plain` and the boost flight include the cloud pass, which the WebGPU path does not draw yet.

Not verified:

- Detached pieces, voxel water, particles and the brush on WebGPU: written, never seen (no capture digs or pours).
- The GPU milliseconds: three's timestamp query swings between 3 and 26 ms for the same view from run to run while frames hold 8.3 ms. Not trusted.
- Cap risers against a riser lying in the cut plane: caps stand 1/64 cube behind the plane to stay clear of it; looked at in stills only, not in motion.
- A level whose box does not hold the finer one's (`levelSquares`): reasoned and reviewed, not seen in a capture.
- Linux, Windows, and three's WebGL 2 fallback backend.

Missing on the WebGPU path: sun shadows, clouds, cloud shadows, precipitation, snow on cube tops, culling inside a batch, fog on three's own materials (particles, brush), specular from material roughness, a colour per liquid for voxel water (one colour for all, magma included), the clear colour.

Known faults in the picture: sunlit tops against shaded risers still stripe slopes seen against the sun; a riser one cube high under a covered top shows the bed's material, which adds to it (engine's choice, section 8).

The WebGL path is unchanged and still the default.

## 3. In progress

Nothing is half-edited; the tree builds and type-checks. The last change applied an independent review (level squares that need not be nested, caps clear of coplanar risers, precision of a voxel's cell far from the anchor, `renderer.init()` failure reported).

## 4. Next steps

1. Sun shadows from the cube batches: two orthographic depth passes (near, 120 km) with a depth-only material that shares `faceNodes().corner`; sample in `cubeSurface`. This is what will calm the stripes on shaded slopes and it is required before the flip.
2. Snow on tops from `CloudField.snowLadder` and `snowLine` (rule in section 6), evaluated per vertex, a cube whole or not by its hash.
3. Dig, pour and drop something in a capture or by hand on the WebGPU path: pieces, voxel water, particles, brush.
4. Watch level boundaries in motion (walk and boost) for popping and for caps flickering; widen nothing before seeing it.
5. Put the face layout's shifts in one constant file (they are literals in `packFaces.ts`, `faceNodes.ts`, `levelClip.ts`, `cubeSurface.ts`, `capFaces.ts`) and take the chunk side from `WorldInfo.cubes.side` (it is 32 in `capFaces.ts` and `FaceBatch.ts`).
6. The flip: make WebGPU the default, delete the WebGL path (list in section 5), give the scene a type of its own in `scene/type/` (it is `ReturnType<typeof createWorldScene>` today), drop `session/fn/cubeWorlds.ts` once the engine makes cubes only, merge the two `capture/pair-*.json` plans.
7. After the flip: clouds, cloud shadow, precipitation, particles as node materials, trusted GPU timings (the cloud pass cost 9 to 16 ms on WebGL; fix that in the port).

## 5. Decisions and why

- One batch per cube size (`cubes/service/FaceBatch.ts`), not one mesh per chunk. Per-chunk meshes cost 10 ms of CPU a frame for 1150 draws in three's WebGPU renderer; batches cost 0.4 ms. The price is no frustum culling inside a batch.
- GPU face layout is four `uint32` words with sixteen-bit positions counted from the batch's anchor (`cubes/fn/packFaces.ts`), not the engine's two words. A face then needs no per-chunk uniform or lookup, and debris and water lists, whose positions do not fit six bits, use the same layout.
- The level is carried in each face's bits and the level squares in one uniform array (`cubes/type/LevelTable.ts`), so every level shares one material.
- A level's box counts by its footprint at any height, because a level holds whole columns of chunks. The contract words the box as a cube; confirm with the engine stream (section 8).
- Level boundary (`cubes/fn/levelSquares.ts`, `capFaces.ts`): the finer square ends on the coarser level's lattice, inside the finer box. The coarser level is cut there and closed by cap risers under every top that reaches the cut from outside, eight cubes deep: the cross-section of its own ground. Without them a ray that enters the cut below a higher coarse top never leaves solid ground and the pixel shows sky. Rejected: a dithered band (it shows the background through crests, as white dots), drawing back faces as a stand-in cap (a ray into rising ground meets none), sinking the coarse level by vertex displacement (greedy quads crack at T-junctions), a depth bias (fails at grazing angles).
- A face on a square's side goes with its cube: the clip tests a point a hundredth of a cube inside the face. A level is drawn a sixteenth of a coarser cube past its square so no crack opens; sheets of water are not, they would blend twice.
- Light: hemisphere of sky over ground plus the sun, flat per face. The ground's light is the bounce of what falls on level ground (`GroundBounce` in `sky/constant/air.ts`), because half the faces of a world of cubes are upright; the old fixed brown left shaded risers black.
- Until clouds are drawn on WebGPU the light is the clear sky's (`Daylight.cover` is not called), so sky and ground agree.
- Octaves of ground variation finer than the cube they are drawn on are left out; they would be noise from cube to cube.
- Deep water is opaque (visibility about 25 m); the old 20 % see-through showed a sunlit bed 2 km down. No shore line is drawn: the `fwidth` line of the smooth water fires on every step of a cube bed.
- Particles and the brush keep three's own materials for now; `CubeStage` puts a hemisphere and a directional light in the scene for them alone.

Traps:

- `DynamicDrawUsage` on an attribute makes three's WebGPU renderer upload it every frame (56 MB a frame here). Leave attributes static; `needsUpdate` with `addUpdateRange` still works.
- `viewportDepthTexture()` copies into a `depth24plus` texture and fails against the float depth buffer that `reversedDepthBuffer` uses. Pass a `DepthTexture(1, 1, FloatType)`: `new ViewportDepthTextureNode(screenUV, null, depth)`.
- A `color` uniform is typed to multiply by floats only; write `vector.mul(colour)`, not `colour.mul(vector)`.
- TSL `mod` is avoided; `groundTone.ts` wraps by hand so negatives wrap as in GLSL.
- Adding the whole ground anchor (up to 32 768 m) to a position before dividing by a voxel's edge costs the cell its last bits; `cubeSurface.ts` adds only the anchor's place within one repeat of the grain.
- A caps run that is removed and added again at every step of the square leaves dead instances that are still sent through the vertex stage; `FaceBatch.cap` writes over one kept run.
- Every stream builds into and captures from the one `@game/dist`. Keep the tree building at all times; Vite does not type-check and a missing import only fails at run time.
- The contract moves: on 26 Nov it changed from nine `int16` a face to `Quads` during this work. Read `GameBridge.ts` before trusting a recording or this file.
- The engine's landing and the planet's surface move between engine builds. Pin `world.spawn` or a shot `position` for like-for-like captures.
- The capture driver takes a shot as `stalled` after 8 s without arrivals; every shot of both pair runs is marked so, and `dataLevels` in the record says how much stand-in data it shows.

Deleted at the flip (WebGL path): `scene/service/WorldScene.ts`, `WorldStage.ts`, `VoxelTerrain.ts`, `VoxelOcclusion.ts`; `scene/fn/voxelSurface.ts`, `voxelGeometry.ts`, `meanNormals.ts`, `looseness.ts`, `surfaceTone.ts` (keep `renderOriginStep`); `scene/constant/ground.ts`; `scene/type/*`; `terrain/**`; `sky/service/Sky.ts`, `sky/fn/dome.ts`, `sky/constant/skyRadiance.ts`, `airFade.ts`, `shadow.ts`, `sky/type/Air.ts`; `SkyLight.groundAmbient`; `water/fn/naturalWater*.ts`, `water/service/NaturalWater.ts`, `WaterScene.ts`; `diagnostics/service/GpuTimer.ts`; and `clouds/**`, `weather/**` rendering once ported. `map/**` draws through its own WebGL 2 canvas (`map/service/ChartSurface.ts`), its text and markers are DOM, and nothing of it is ported; keep its lines in `index.ts` and the `#map` section of `index.html` (listed in `docs/handoff/map.md`).

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
- Views hold up to 3.5 M quads (levels 7 and 8 hold about 1 M each at a land spot); a way to cull, or fewer hidden faces, would help weaker GPUs.
- `Frame.pending.patches` and `ready` are what a capture waits on; confirm they cover cube chunks.
- Confirm that a level holds every chunk over each square of ground inside its box's footprint, at any height: the renderer draws a level by footprint.
- A riser one cube high under a covered top carries the bed's material; on gentle covered slopes every step then shows a dark band. Should the top cube's own sides carry the cover?

Lines added to shared files:

- `@game/src/index.ts`: imports of `createCubeScene` and `cubeWorlds`; in `compose()` the `cubes` constant read from `location.search` and the `scene` choice (`createCubeScene(canvas, showError)`); `cubeWorlds(window.derivean)` passed to the session; `scene.covered = map.open;` before `session.tick` in `draw` (asked for by the map stream); `backend: scene.backend, draw: scene.timings()` in `metrics.paint`; `backend: scene.backend, drawMs: scene.timings()` in `measure`.
- `@game/src/capture/service/CaptureDriver.ts`: `|| frame.cubes?.added.length` in the arrival test, so a cube world is not taken as stalled while it streams.
- `@game/main/index.mjs`: `DERIVEAN_RENDERER=webgpu` sets `renderer.search`; `loadURL(renderer.href)` replaces `loadFile`, and the sender check compares against the same address.

The M key and `map/**` are the map stream's. The window title stays "DeRivean · Fable".

## 7. How to build, run and verify

- Build: `npm run build` in `@game` (Vite; it does not type-check). Types: `npx tsc --noEmit` in `@game`.
- Real app: from the worktree root, `DERIVEAN_CAPTURE=<plan.json> @game/node_modules/.bin/electron @game`; add `DERIVEAN_RENDERER=webgpu` for the cube path. It uses `.out/addon/derivean.node` as built by the engine stream. Engine start takes 2 to 8 minutes under load; the ten shots of a pair plan took about 12 minutes. Plans: `@game/capture/look.json` (37 shots, the look's judge), `@game/capture/pair-smooth.json` and `pair-cubes.json` (the same ten shots at a pinned land spot, one output folder each). Shots and records land in the plan's `out`.
- The capture pair, smooth against cubes, same view: `.out/playwright/look/pair-smooth/` and `.out/playwright/look/pair-cubes/` (`300m-north.png` is the mountain view; also four ground shots, 300 m plain and south, 2 km, 8 km, and the boost flight).
- Replay, the fast loop (about a minute, no engine): tooling in `.out/renderer-replay/` (scratch, untracked). Record once with `ELECTRON_RUN_AS_NODE=1 @game/node_modules/.bin/electron .out/renderer-replay/record.cjs <worktree> <outDir> 1 <spawnX> <spawnZ>` (ground, 300 m and 2 km above a spot; about 7 minutes), then `REPLAY_DATA=<outDir> .out/renderer-replay/run.sh .out/renderer-replay/plan-land.json`; edit the paths at the top of `run.sh` first. It runs the built app against the recorded frames through the real preload and capture driver and prints draws, triangles, backend and timings per shot. A recording is stale as soon as the contract changes. Land spot used: spawn `[-2521902, -4671906]` (a lake at 735 m among hills, seed 42, 4.6 Gyr).
- Frame statistics are not trustworthy above load about 40; say the load with every number. Close every instance you start; other streams run instances too, so check a process's parent and working directory before ending it.
- Good looks like: the same view in cubes at every distance; walls, ledges and crests crisp; strata as bands on far cliffs; cubes told apart by light and corner occlusion under overcast; no shimmer where risers are near a pixel; no holes at level boundaries; aerial perspective to the limit of view; water as flat sheets with a shore; snow above a snow line. Compare with `docs/reference-landscapes.md`.
- WebGL baseline for comparison: `.out/playwright/look/` (smooth patches; boost flight p50 8.3 ms, p95 16.6 ms, 560 to 630 draws, about 1 M triangles, cloud pass 6 to 17 ms GPU).

## 8. Open questions

- Is the GPU within budget on weaker machines? Frames hold 8.3 ms here with up to 6.9 M triangles unculled; the timestamp query is not trusted. Splitting a batch by quadrant or indirect multi-draw are the candidates if not.
- Does a level hold whole columns of chunks inside its box's footprint (section 5)? If not, the top and bottom of a box need the same cut and cap as its sides.
- Risers of a one-cube step under a covered top: bed or cover (section 6)?
- The cut between levels moves in steps of one coarser cube as the eye moves. Whether that reads as popping has not been watched in motion.
