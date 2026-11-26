# Renderer hand-over

Owner of this file: the renderer stream (`@game/src/**` except `map/**`, `@game/main/index.mjs`, `@game/main/capture.mjs`, `@game/index.html`, `@game/capture/**`). It describes the present.

## 1. Goal

The owner's terms:

- "We are supposed to draw a world of cubes, but the render — the LODs — is rounded. Switch everything to cubes or approximations of cubes; the sharp edges will stand out as they should, even in the distance." Far terrain is the voxel world at coarser cube sizes, to the limit of view, drawn by one pipeline with the near voxels. No smooth height meshes, no interpolated normals on terrain.
- "Switch it to WebGPU." three's `WebGPURenderer` (`three/webgpu`) with TSL node materials. Compute is welcome for presentation, forbidden for anything that decides the world.
- Still binding: no heat-map look; colour only from the simulation's materials; view to at least 250 km with aerial perspective; frame time in boost flight must hold; clouds causal, from the simulated water (`brief/clouds.md`).

The flip is done (26 Nov, fourth run): WebGPU is the only renderer and the WebGL path is deleted. Its last state is in commit `c131234` (`git show c131234:@game/src/clouds/...`), the source to port clouds, cloud shadows and precipitation from.

## 2. State now

One renderer: `scene/service/CubeScene.ts` (type `scene/type/WorldView.ts`) on three's `WebGPURenderer`. `tsc --noEmit` passes and `npm run build` builds. Verified on macOS, Apple GPU, Electron 45.5, three 0.187, by replays of recorded engine frames (`rec-stills`, section 7; backend `WebGPU`, no page warnings or errors) and, before this run, by the live pair capture:

- Voxel chunks and far cube levels 2 to 13 from `Frame.chunks[].quads` and `Frame.cubes`, one material, one draw per cube size: 33 to 38 draws for 2.3 to 5.5 M triangles.
- Level squares from `Frame.cubes.levels[].box` and `Frame.voxelBox`, cut on the coarser level's lattice and closed by cap risers. Flat normals, corner occlusion, per-cube tone, world-anchored ground variation.
- Sun shadows over the whole view (`shadow/**`): four cascades of depth maps seen from the sun, four levels of cubes to a cascade, the last over the whole view; drawn from the cube batches' second meshes and redrawn only when the eye, the sun or the casters move. Seen working in a debug view of the shadow term (shadows of pillars and hills, nothing on lit faces) and in plain shading; low-sun replays at 300 m and 2 km.
- Snow on tops from `CloudField.snowLadder` and `snowLine` (`snow/**`), read as the simulation reads it. Cubes under 0.5 m are covered whole or not by their hash, cubes of 1 m and more take the snow's share of their colour (section 5). Seen in replays of a midwinter recording: snow on flats, bare risers, no flecks at distance.
- Standing water sheets from `CubeChunk.water`, shore depth from a copy of the scene depth.
- Sky background, aerial perspective, exposure, plain shading (L).
- F3 and every shot record say the backend and the draw's CPU and GPU milliseconds; the CPU time includes the shadow passes (0.45 to 0.6 ms in replays).
- The world is not drawn while the map is open (`scene.covered = map.open` in `index.ts`).

Replays of `rec-stills` (load 30 to 130; read frame times as "holds", not as numbers): frame p50 8.3 ms everywhere, p95 9 to 50 ms (the replay feeds a whole recording into the first frames), draw CPU 0.45 to 0.6 ms with shadows, GPU 3.4 to 9 ms (timestamp query, not trusted).

Not verified:

- Shadows and snow in a live run (section 3).
- Shadows in motion: cascade redraws, the snap of a cascade's centre, popping of shadows at cascade borders.
- Detached pieces, voxel water, particles and the brush: written, never seen. The `dig-*` and `place-*` shots of `@game/capture/cubes-live.json` dig and place at the pinned spot (the capture driver's new `brush` action).
- Level boundaries in motion (popping, caps flickering).
- Linux, Windows, and three's WebGL 2 fallback backend.

Missing: clouds, cloud shadows, precipitation (deleted with the WebGL path; `veil` and `cloudCost` are stubs, the light is the clear sky's); culling inside a batch; fog on three's own materials (particles, brush); specular from material roughness; a colour per liquid for voxel water (one colour for all, magma included).

Known faults in the picture:

- Thin pillars of loose silt and sand one cube wide and up to hundreds of metres high stand all over the pinned land spot in the 03:03 engine data (`rec-stills`: 2347 riser quads one cube wide and 16 or more high at level 5, materials Silt, Sand, Gravel). They are in the engine's quads, not made by the renderer; reported to the lead.
- Sunlit tops against shaded risers still stripe slopes seen against the sun; shadows calm it only where a slope is shadowed.
- A riser one cube high under a covered top shows the bed's material (engine's choice, section 6).
- At 8 km the far levels read as a carpet of tops and dark risers; on gentle ground every step of a coarse cube shows its riser.

## 3. In progress

Fourth run (26 Nov, from 08:50). Done in it: sun shadows, snow, the face layout constants, the capture `brush` action, and the flip (WebGL path deleted). The tree builds and type-checks; nothing is half-edited. Running: a live capture with `@game/capture/cubes-live.json` (started 09:24 on the flipped build) into `.out/playwright/renderer/live-s42-x-2521902-z-4671906-2026-11-26/`. A first attempt at 09:03 aimed at the look plan's old range position, which is sea on today's planet; it was stopped. No independent review of this run's changes yet.

## 4. Next steps

1. Look at the live run's shots (`.out/playwright/renderer/live-s42-x-2521902-z-4671906-2026-11-26/`): shadows at hour 16, snow, the `dig-*`/`place-*` shots (pieces, particles, voxel water, brush), the six `move-40m-n-*` stills one second of flight apart (level cut and shadow cascade steps), the boost flight's frame times. Fix what is wrong.
2. Find a real range by search (`terra relief --at cold-range` or `humid-range` prints coordinates) and shoot it at a low sun: do shadows give it mass?
3. Clouds, cloud shadow, precipitation on WebGPU, causal from the simulation's `CloudField` (`brief/clouds.md`). Port from `c131234:@game/src/clouds/**` and `weather/**`: the field textures (`CloudField.ts`, `encodeClouds.ts`), the noise volumes, the march / resolve / compose passes (GLSL; rewrite as TSL or `wgslFn` in a post-processing pass after the scene, reading the scene depth), cloud shadow as a factor in `faceLight` beside `sunlit`, `Daylight.cover` from the cloud over the eye, and particles of rain and snow. Time the GPU properly: the old cloud pass cost 9 to 16 ms on WebGL.
4. Fog for particles and the brush (node materials of their own); a colour per liquid for voxel water.
5. Put the chunk side into `WorldInfo.cubes.side` everywhere (`FaceBatch` takes it; check `capFaces` callers) and give the engine's `Quads` layout a constant beside `cubes/constant/layout.ts` if it changes again.
6. Drop `session/fn/cubeWorlds.ts` once the engine makes cubes only; merge `pair-cubes.json` into the look plan.

## 5. Decisions and why

- One batch per cube size (`cubes/service/FaceBatch.ts`), not one mesh per chunk. Per-chunk meshes cost 10 ms of CPU a frame for 1150 draws in three's WebGPU renderer; batches cost 0.4 ms. The price is no frustum culling inside a batch.
- GPU face layout is four `uint32` words with sixteen-bit positions counted from the batch's anchor (`cubes/fn/packFaces.ts`), not the engine's two words. A face then needs no per-chunk uniform or lookup, and debris and water lists, whose positions do not fit six bits, use the same layout.
- The level is carried in each face's bits and the level squares in one uniform array (`cubes/type/LevelTable.ts`), so every level shares one material.
- A level's box counts by its footprint at any height, because a level holds whole columns of chunks. The contract now says so: every level is whole over its box's footprint at any height.
- Level boundary (`cubes/fn/levelSquares.ts`, `capFaces.ts`): the finer square ends on the coarser level's lattice, inside the finer box. The coarser level is cut there and closed by cap risers under every top that reaches the cut from outside, eight cubes deep: the cross-section of its own ground. Without them a ray that enters the cut below a higher coarse top never leaves solid ground and the pixel shows sky. Rejected: a dithered band (it shows the background through crests, as white dots), drawing back faces as a stand-in cap (a ray into rising ground meets none), sinking the coarse level by vertex displacement (greedy quads crack at T-junctions), a depth bias (fails at grazing angles).
- A face on a square's side goes with its cube: the clip tests a point a hundredth of a cube inside the face. A level is drawn a sixteenth of a coarser cube past its square so no crack opens; sheets of water are not, they would blend twice.
- Light: hemisphere of sky over ground plus the sun, flat per face. The ground's light is the bounce of what falls on level ground (`GroundBounce` in `sky/constant/air.ts`), because half the faces of a world of cubes are upright; the old fixed brown left shaded risers black.
- Until clouds are drawn the light is the clear sky's (`Daylight.cover` is not called), so sky and ground agree.
- Octaves of ground variation finer than the cube they are drawn on are left out; they would be noise from cube to cube.
- Deep water is opaque (visibility about 25 m); the old 20 % see-through showed a sunlit bed 2 km down. No shore line is drawn: the `fwidth` line of the smooth water fires on every step of a cube bed.
- Sun shadows (`shadow/service/SunShadow.ts`, `shadow/constant/cascades.ts`): the casters are the batches' second meshes in a scene of their own, each on the layer of its cascade. A map holds only faces that look away from the sun (`BackSide`), where a ray leaves solid ground, so lit faces never shadow themselves and no slope bias is needed. A point takes the shadows of the cascade whose square it lies in and, from coarser cascades, only beyond that square, moved along its ray to where it leaves the square (`shadow/fn/sunlit.ts`): a coarse map holds the same ground again as larger cubes. A cascade is redrawn when the eye has moved an eighth of its half width, the sun 0.002 rad, or its casters changed, one cascade a frame, the coarse ones seldom. Map centres move in whole texels. Depth is reversed as on screen.
- Snow (`snow/**`, `cubes/fn/cubeSurface.ts`): the ladder is evaluated per vertex of a top (tops only, as the contract says). A cube's top stands for the ground under it: a small cube is covered or bare, a cube of a metre or more takes the snow's share of its colour, which is what the small cubes average to. Per-cube white or bare at every size scattered the far view with white flecks.
- The face layout's words, shifts and widths live in `cubes/constant/layout.ts`; `packFaces` writes and `faceNodes` reads only through it.
- The flip deleted the WebGL renderer, patches, smooth water, the dome sky and the WebGL clouds and precipitation in one change; Git holds them (`c131234`).
- Particles and the brush keep three's own materials for now; `CubeStage` puts a hemisphere and a directional light in the scene for them alone.

Traps:

- `DynamicDrawUsage` on an attribute makes three's WebGPU renderer upload it every frame (56 MB a frame here). Leave attributes static; `needsUpdate` with `addUpdateRange` still works.
- `viewportDepthTexture()` copies into a `depth24plus` texture and fails against the float depth buffer that `reversedDepthBuffer` uses. Pass a `DepthTexture(1, 1, FloatType)`: `new ViewportDepthTextureNode(screenUV, null, depth)`.
- A `color` uniform is typed to multiply by floats only; write `vector.mul(colour)`, not `colour.mul(vector)`.
- TSL `mod` is avoided; `groundTone.ts` wraps by hand so negatives wrap as in GLSL.
- Adding the whole ground anchor (up to 32 768 m) to a position before dividing by a voxel's edge costs the cell its last bits; `cubeSurface.ts` adds only the anchor's place within one repeat of the grain.
- A caps run that is removed and added again at every step of the square leaves dead instances that are still sent through the vertex stage; `FaceBatch.cap` writes over one kept run.
- Every stream builds into and captures from the one `@game/dist`. Keep the tree building at all times; Vite does not type-check and a missing import only fails at run time. For debug views, copy `@game` into your scratch directory (rsync without `node_modules` and `dist`, link `node_modules`), patch and `vite build` there, and replay with `REPLAY_ROOT` pointing at that copy; never build debug code into the shared `dist`.
- The scratch directory is shared by all streams; keep files in a subdirectory of your own.
- The contract moves: on 26 Nov it changed from nine `int16` a face to `Quads` during this work. Read `GameBridge.ts` before trusting a recording or this file.
- The engine's landing and the planet's surface move between engine builds. Pin `world.spawn` or a shot `position` for like-for-like captures.
- The capture driver takes a shot as `stalled` after 8 s without arrivals; every shot of both pair runs is marked so, and `dataLevels` in the record says how much stand-in data it shows.


## 6. Contracts with other streams

Engine (`@game/contract/GameBridge.ts`, read it for the truth):

- `WorldOptions.cubes: true` starts a world of cubes; `session/fn/cubeWorlds.ts` adds it to every start.
- `Quads`: two `uint32` a quad, chunks 32 cubes a side. Word 0: plane (bits 0 to 5), lowU (6 to 11), lowV (12 to 17), sizeU − 1 (18 to 22), sizeV − 1 (23 to 27), axis (28 to 29), positive normal (30). Word 1: material (0 to 7), corner occlusion (8 to 15, two bits a corner from (lowU, lowV), (highU, lowV), (lowU, highV), (highU, highV)).
- `ChunkMesh.quads` is the ground of a voxel chunk; `ChunkMesh.faces` (`WorldInfo.faceWords` shorts a face) stays for water meshes and debris.
- `CubeChunk { id, level, edge, origin, quads, water: { level, quads }[] }`; sheet quads have plane 0 and are drawn at `level` metres.
- `Frame.cubes.levels[]`: `{ level, edge, wanted, shown, box }`. A level is drawn inside its box and outside the next finer box; the finest outside `voxelBox`, whose footprint counts at any height.
- Snow, drawn: `CloudField.snowLadder` (kg/m², rungs at `WorldInfo.snowLadder` metres above `seaLevel`), `snowLine` per cell. Nothing below the snow line, linear from there to the first rung above and between rungs, end rungs held, bilinear between four cells. Tops only.

Asked of the engine stream, not yet answered:

- One chunk can carry two sheets at the same water level (seen at 35.5 m over the sea), which blend twice.
- Views hold up to 3.5 M quads (levels 7 and 8 hold about 1 M each at a land spot); a way to cull, or fewer hidden faces, would help weaker GPUs.
- `Frame.pending.patches` and `ready` are what a capture waits on; confirm they cover cube chunks.
- Pillars of silt and sand one cube wide and up to hundreds of metres high in the 03:03 data (section 2): an engine or ground defect.
- A riser one cube high under a covered top carries the bed's material; on gentle covered slopes every step then shows a dark band. Should the top cube's own sides carry the cover?

Lines added to shared files:

- `@game/src/index.ts`: import of `createCubeScene` and `cubeWorlds`; `const scene = createCubeScene(canvas, showError)` in `compose()`; `cubeWorlds(window.derivean)` passed to the session; `scene.covered = map.open;` before `session.tick` in `draw` (asked for by the map stream); `levels: scene.levelCounts()`, `backend`, `draw: scene.timings()` in `metrics.paint`; `chunksByLevel`, `backend`, `drawMs` in `measure`. The rest of `index.ts` besides the map's lines is this stream's.
- `@game/src/capture/service/CaptureDriver.ts`: `|| frame.cubes?.added.length` in the arrival test; the `brush` phase.
- `@game/main/index.mjs`: `loadURL(renderer.href)` replaces `loadFile`, and the sender check compares against the same address. `DERIVEAN_RENDERER` is gone.

The M key and `map/**` are the map stream's. The window title stays "DeRivean · Fable".

## 7. How to build, run and verify

- Build: `npm run build` in `@game` (Vite; it does not type-check). Types: `npx tsc --noEmit` in `@game`.
- Real app: from the worktree root, `DERIVEAN_CAPTURE=<plan.json> @game/node_modules/.bin/electron @game`. It uses `.out/addon/derivean.node` as built by the engine stream. Engine start takes 2 to 8 minutes under load. Plans: `@game/capture/look.json` (37 shots, the look's judge), `pair-cubes.json` (ten shots at the pinned land spot), `cubes-live.json` (the pinned spot from the ground, 300 m, 2 km and 8 km at hours 12 and 16, digging and placing, six stills one second of flight apart, a boost flight). Shots and records land in the plan's `out`.
- Capture shots may `brush: { frames, place?, radius?, seconds? }`: after settling, hold dig (or place basalt) along the view for that many frames and shoot `seconds` after the first stroke.
- Replay, the fast loop (about a minute, no engine): tooling in `.out/renderer-replay/` (scratch, untracked). Record once with `ELECTRON_RUN_AS_NODE=1 @game/node_modules/.bin/electron .out/renderer-replay/record2.cjs ...` (see `spec-stills.json`; about 18 minutes), then `REPLAY_ROOT=<worktree> REPLAY_DATA=.out/renderer-replay/rec-stills REPLAY_PLAN=<plan.json> @game/node_modules/.bin/electron .out/renderer-replay`. A plan lists shots `{ name, segment, yaw, pitch, plain?, sun? }` (segment 0 ground, 1 at 300 m, 2 at 2 km, 3 at 8 km; `sun` a unit vector towards the sun overrides the recorded one) and `out`. `rec-stills` was recorded at 03:03 on 26 Nov at spawn `[-2521902, -4671906]` (seed 42, 4.6 Gyr, midwinter noon, sun 35° from the north); it is stale once the contract's quads change.
- Frame statistics are not trustworthy above load about 40; say the load with every number. Close every instance you start; other streams run instances too, so check a process's parent and working directory before ending it.
- Good looks like: the same view in cubes at every distance; walls, ledges and crests crisp; strata as bands on far cliffs; cubes told apart by light and corner occlusion under overcast; shadows that give ranges their mass at a low sun; no shimmer where risers are near a pixel; no holes at level boundaries; aerial perspective to the limit of view; water as flat sheets with a shore; snow above a snow line. Compare with `docs/reference-landscapes.md`.
- Captures of this run: `.out/playwright/renderer/` (`shadow-*`, `lowsun-*`, `snowmean-*`, `flip-*` from replays; `live-*` from the real app). WebGL baseline before the flip: `.out/playwright/look/` and `look/pair-smooth/`.

## 8. Open questions

- Is the GPU within budget on weaker machines? Frames hold 8.3 ms here with up to 6.9 M triangles unculled plus up to one shadow cascade a frame; the timestamp query is not trusted. Splitting a batch by quadrant or indirect multi-draw are the candidates if not.
- Shadow map memory: three 4096² and one 2048² float depth maps (about 210 MB with their colour targets). Generous by the owner's rule; pull down only if a machine cannot hold it.
- Risers of a one-cube step under a covered top: bed or cover (section 6)?
- The cut between levels and the shadow cascades move in steps as the eye moves. Whether that reads as popping has not been watched in motion.
- Should risers of coarse cubes, which stand for sloping ground, take some of the snow? The contract says tops only.
