# Renderer hand-over

Owner of this file: the renderer stream (`@game/src/**` except `map/**`, `@game/main/index.mjs`, `@game/main/capture.mjs`, `@game/index.html`, `@game/capture/**`). It describes the present.

## 1. Goal

The owner's terms:

- "We are supposed to draw a world of cubes, but the render — the LODs — is rounded. Switch everything to cubes or approximations of cubes; the sharp edges will stand out as they should, even in the distance." Far terrain is the voxel world at coarser cube sizes, to the limit of view, drawn by one pipeline with the near voxels. No smooth height meshes, no interpolated normals on terrain.
- "Switch it to WebGPU." three's `WebGPURenderer` (`three/webgpu`) with TSL node materials. Compute is welcome for presentation, forbidden for anything that decides the world.
- Still binding: no heat-map look; colour only from the simulation's materials; view to at least 250 km with aerial perspective; frame time in boost flight must hold; clouds causal, from the simulated water (`brief/clouds.md`).

The flip is done (26 Nov, fourth run): WebGPU is the only renderer and the WebGL path is deleted; its last state is in commit `c131234`. Clouds, cloud shadows and precipitation are ported from it to WebGPU.

## 2. State now

One renderer: `scene/service/CubeScene.ts` (type `scene/type/WorldView.ts`) on three's `WebGPURenderer`. `tsc --noEmit` passes and `npm run build` builds. Verified on macOS, Apple GPU, Electron 45.5, three 0.187, by replays of recorded engine frames (section 7; backend `WebGPU`, no page warnings or errors) and by live captures (`cubes-live.json` before the clouds, `clouds-live.json` with them, `land-before`/`land-after` for the land light):

- Voxel chunks and far cube levels 2 to 13 from `Frame.chunks[].quads` and `Frame.cubes`, one material, one draw per cube size: 33 to 38 draws for 2.3 to 5.5 M triangles.
- Level squares from `Frame.cubes.levels[].box` and `Frame.voxelBox`, cut on the coarser level's lattice and closed by cap risers. Flat normals, corner occlusion, per-cube tone, world-anchored ground variation.
- Sun shadows over the whole view (`shadow/**`): four cascades of depth maps seen from the sun, four levels of cubes to a cascade, the last over the whole view; drawn from the cube batches' second meshes and redrawn only when the eye, the sun or the casters move. Seen working in a debug view of the shadow term (shadows of pillars and hills, nothing on lit faces) and in plain shading; low-sun replays at 300 m and 2 km.
- Snow on tops from `CloudField.snowLadder` and `snowLine` (`snow/**`), read as the simulation reads it. Cubes under 0.5 m are covered whole or not by their hash, cubes of 1 m and more take the snow's share of their colour (section 5). Seen in replays of a midwinter recording: snow on flats, bare risers, no flecks at distance.
- Standing water sheets from `CubeChunk.water`, shore depth from a copy of the scene depth.
- Clouds (`clouds/**`), ported from the WebGL version: the engine's `CloudField` bands in a float array texture, blended between snapshots; shape and detail noise written once by compute shaders into 3D storage textures, the cover levels measured on the GPU and read back; the march (`fn/marchNode.ts`) and temporal resolve (`fn/resolveNode.ts`) in WGSL at half size, composed over the scene by depth (`service/CloudPass.ts`). The scene is now drawn into the pass's 4× multisampled half-float target; tone mapping happens once, on the way to the screen. Seen in replays of the midwinter recording: a cumulus field next to an overcast deck from the ground, inside the band at 2.8 km and from above at 8.8 km, right side up, no page errors.
- Cloud shadows (`clouds/service/CloudShadow.ts`): a 96 km map of sunlight through the same density, redrawn five times a second, read in `shadow/fn/sunlit.ts` beside the sun's maps and by both water surfaces; beyond the map, and for three's own lights, the cloud over the eye. The sun is no longer dimmed by the cover in `Daylight`. Seen only from 8.8 km (ground in a gap darker on the side the cloud stands); not judged from the ground.
- Rain and snow (`weather/**`): instanced streaks at the simulated rates and wind. Each precipitation geometry carries its own `seed` attribute, so disposing the geometry frees its GPU buffer (three 0.187 frees an attribute's buffer on `attribute.dispose()` or its geometry's). Snow seen falling over the sea in `clouds-live-*/snow-ground-n.png`.
- Land light (`cubes/fn/landNormal.ts`, `cubes/service/LevelHeights.ts`, `cubes/constant/land.ts`): a face of a far cube takes up to 75 % of its light as the slope of the land round its cube, measured on that level's own column heights (±3 and ±8 columns, averaged), with the sun as it reaches the top of that column; ramped in by cube size from 2 m to 16 m (by doublings), and none where the land is steeper than about 45°. Each level keeps the top height of every column it holds in a ring of 16 × 16 chunk squares in a storage buffer, with the square each place holds; the nearer square keeps a place two squares share. Every fragment of a cube takes one value; nothing is interpolated. WebGPU only (the colour node checks the backend when it is built). Seen live at the pinned spot at noon and 15 h, clear sky (`land-before-*` against `land-after-*`: 2 km, 8 km down, along and back, 30 km): the dark riser stripes of every coarse step are gone, risers read a little darker than tops, shaded sides of hills read as one mass, steep steps keep dark risers, cube edges and silhouettes stay sharp. The far view is paler and flatter than before; whether 0.75 is the right share is the owner's call. Costs nothing measurable: 240 back-to-back frames with it off and on, 2 km view 6.73/6.81/6.77/6.77 ms, 8 km view 5.05/5.01/5.05/5.01 ms (load about 50).
- Snow bounce: the light a face gets from the ground below takes the snow's share of the snow's colour, read at the face's middle, so shaded risers in snow are lit by the snow in front of them. Plain shading keeps its neutral bounce.
- The cold range (`[6512640, 8462336]`, ground 5.6 km, snow on every top) at a low sun, 11.5° up (`coldrange-lowsun-*`, hours 5.5 and 18.5; the hour is local solar time, and at this latitude 9 h and 15.5 h put the sun 42° to 47° up, `coldrange-*`): slopes turned to the sun white, slopes turned away blue-grey, hillsides and the lake basin read as volumes at 2 km and 8 km, cube edges sharp. Shaded snow sits at about 0.7 of lit snow (ground luma p10 136 to 152 against p90 191 to 209). Under the high sun of the 9 h and 15.5 h shots the range reads pale and flat (shaded at 0.8 to 0.9 of lit). Long cast shadows from ridges are not plainly visible; whether the blue-grey fields are cast shadow or slope shading was not separated (no debug view in a live capture). The rock of ranges (commit 38b4939) shows only on the skyline risers here.
- Digging and placing on WebGPU: holes, pieces and placed basalt seen in the live capture's `dig-*` and `place-*` shots.
- Sky background, aerial perspective, exposure, plain shading (L).
- F3 and every shot record say the backend and the draw's CPU and GPU milliseconds; the CPU time includes the shadow passes (0.45 to 0.6 ms in replays).
- The world is not drawn while the map is open (`scene.covered = map.open` in `index.ts`).

Replays of the 03:03 recording (load 30 to 130; read frame times as "holds", not as numbers): frame p50 8.3 ms everywhere, p95 9 to 50 ms (the replay feeds a whole recording into the first frames), draw CPU 0.45 to 0.6 ms with shadows, GPU 3.4 to 9 ms (timestamp query, not trusted).

Not verified:

- Cloud shadows from the ground in motion; clouds over a long turning sequence (only stills taken while turning).
- Shadows and level boundaries in motion: the live capture's six stills one second apart at walking speed show no hole or flicker, but stills cannot show popping.
- Voxel water and particles after digging (the dig shots show the hole and pieces, no water).
- The cloud cost on other GPUs. Here, timed as 240 frames drawn back to back with the clouds hidden and shown (`__bench` in a scratch build, load 20 to 30): ground looking up 5.1 → 6.2 ms, 300 m 4.1 → 5.9 ms, inside the band at 2 km 4.0 → 7.7 ms, above at 8 km 3.0 → 6.5 ms. Clouds cost 1 to 3.7 ms. Live boost flights hold p50 8.3 ms, p99 9.3 ms with and without clouds (load about 40). `cloudCost` still reports null; the GPU timestamp is not trusted.
- Linux, Windows, and three's WebGL 2 fallback backend.

Missing: culling inside a batch; fog on three's own materials (particles, brush); specular from material roughness; a colour per liquid for voxel water (one colour for all, magma included).

Known faults in the picture:

- Near voxel steps on gentle snowy ground show as rows of small brown pins (`live-*/move-40m-n-3.png`, lower left): the bare risers of one-voxel steps where the contours cross the lattice, under snow-white tops; the same at the cold range from the ground (`coldrange-*/h15.5-ground-w.png`: grass, soil and limestone risers under covered tops). Snow lies on tops only (section 6); the land light does not reach voxels.
- A riser one cube high under a covered top shows the bed's material (engine's choice, section 6).
- Clouds: a fine diagonal hatch in the deck's darker parts, stronger in stills taken while turning (0.6 rad/s); half-size blockiness on cloud edges after a fast flight; the deck's lower edge ends in a straight line above the horizon.

## 3. In progress

Fourth run (26 Nov, from 08:50). Done in it: sun shadows, snow, the face layout constants, the capture `brush` and `turn` actions, the flip (WebGL path deleted), live captures before and with the clouds, the port of clouds, cloud shadows and precipitation, the measured cloud cost, the land light and the snow bounce (both reviewed independently; findings applied: ring places shared by two squares, the sun for the land term taken at the column top, a wider slope reach, the ramp by doublings, the bounce read at the face middle, the turned yaw sent to the engine and recorded). Also done: `rec-stills` re-recorded from the current tree (11:22, adds a 30 km segment; no riser one cube wide and 8 or more high in it, where the 03:03 recording, kept as `rec-stills-stale-0303`, had 5222 at level 7), and the cold range at a low sun. Nothing is running. The tree builds and type-checks.

## 4. Next steps

1. Judge replays against the new `rec-stills` only; `shadow-*`, `lowsun-*`, `snowmean-*`, `flip-*`, `clouds-*` are stale for world defects. Retake the low-sun and land-light replays from it.
2. At the cold range, separate cast shadow from slope shading (the shadow debug view in a replay recorded there) and check that ridges throw long shadows at a sun 10° up.
3. The owner should judge the land light (`land-before-*` against `land-after-*`); `LandShare` (0.75) is the one number to turn. At 1.0 the far view reads as a smooth shaded relief, which the owner forbids.
4. The pins on near snowy ground: snow on risers of slopes the snow lies on (needs the contract's "tops only" to change, section 6), or the land light carried down to voxels.
5. Clouds: the hatch in the deck while turning (the temporal resolve's dither), blocky cloud edges after fast flight.
6. Fog for particles and the brush (node materials of their own); a colour per liquid for voxel water.
7. Put the chunk side into `WorldInfo.cubes.side` everywhere and drop `session/fn/cubeWorlds.ts` once the engine makes cubes only.

## 5. Decisions and why

- One batch per cube size (`cubes/service/FaceBatch.ts`), not one mesh per chunk. Per-chunk meshes cost 10 ms of CPU a frame for 1150 draws in three's WebGPU renderer; batches cost 0.4 ms. The price is no frustum culling inside a batch.
- GPU face layout is four `uint32` words with sixteen-bit positions counted from the batch's anchor (`cubes/fn/packFaces.ts`), not the engine's two words. A face then needs no per-chunk uniform or lookup, and debris and water lists, whose positions do not fit six bits, use the same layout.
- The level is carried in each face's bits and the level squares in one uniform array (`cubes/type/LevelTable.ts`), so every level shares one material.
- A level's box counts by its footprint at any height, because a level holds whole columns of chunks. The contract now says so: every level is whole over its box's footprint at any height.
- Level boundary (`cubes/fn/levelSquares.ts`, `capFaces.ts`): the finer square ends on the coarser level's lattice, inside the finer box. The coarser level is cut there and closed by cap risers under every top that reaches the cut from outside, eight cubes deep: the cross-section of its own ground. Without them a ray that enters the cut below a higher coarse top never leaves solid ground and the pixel shows sky. Rejected: a dithered band (it shows the background through crests, as white dots), drawing back faces as a stand-in cap (a ray into rising ground meets none), sinking the coarse level by vertex displacement (greedy quads crack at T-junctions), a depth bias (fails at grazing angles).
- A face on a square's side goes with its cube: the clip tests a point a hundredth of a cube inside the face. A level is drawn a sixteenth of a coarser cube past its square so no crack opens; sheets of water are not, they would blend twice.
- Light: hemisphere of sky over ground plus the sun, flat per face. The ground's light is the bounce of what falls on level ground (`GroundBounce` in `sky/constant/air.ts`), because half the faces of a world of cubes are upright; the old fixed brown left shaded risers black.
- Clouds (`clouds/**`): the march, resolve and density are WGSL through `wgslFn`, a near line-for-line port of the GLSL, because the march's loops and early outs read better there than in TSL. Textures are function arguments (WGSL binds them where three declares them); the struct `CloudSky` and the parameters come from one field list (`constant/density.ts`, `fn/cloudArgs.ts`). Every sample uses `textureSampleLevel` or `textureLoad`, which WGSL allows in non-uniform control flow. The cover levels are read with `textureLoad` and blended by hand, so they need no sampler.
- The cloud pass owns the scene's target (multisampled colour and float depth) because it reads both; with clouds hidden it only copies the scene to the screen. The renderer's own antialiasing is off: the canvas only receives that copy.
- Under cloud the sun is dimmed by the clouds' own shadows where the map reaches (96 km), and by the cloud over the eye beyond it and for three's own lights; `Daylight` no longer dims it by the cover as well. The sky light still follows the cover (`Daylight.cover`).
- The noise is written to storage textures by compute shaders; three's WebGL 2 fallback cannot, so it draws no clouds.
- Octaves of ground variation finer than the cube they are drawn on are left out; they would be noise from cube to cube.
- Deep water is opaque (visibility about 25 m); the old 20 % see-through showed a sunlit bed 2 km down. No shore line is drawn: the `fwidth` line of the smooth water fires on every step of a cube bed.
- Sun shadows (`shadow/service/SunShadow.ts`, `shadow/constant/cascades.ts`): the casters are the batches' second meshes in a scene of their own, each on the layer of its cascade. A map holds only faces that look away from the sun (`BackSide`), where a ray leaves solid ground, so lit faces never shadow themselves and no slope bias is needed. A point takes the shadows of the cascade whose square it lies in and, from coarser cascades, only beyond that square, moved along its ray to where it leaves the square (`shadow/fn/sunlit.ts`): a coarse map holds the same ground again as larger cubes. A cascade is redrawn when the eye has moved an eighth of its half width, the sun 0.002 rad, or its casters changed, one cascade a frame, the coarse ones seldom. Map centres move in whole texels. Depth is reversed as on screen.
- Snow (`snow/**`, `cubes/fn/cubeSurface.ts`): the ladder is evaluated per vertex of a top (tops only, as the contract says). A cube's top stands for the ground under it: a small cube is covered or bare, a cube of a metre or more takes the snow's share of its colour, which is what the small cubes average to. Per-cube white or bare at every size scattered the far view with white flecks.
- The face layout's words, shifts and widths live in `cubes/constant/layout.ts`; `packFaces` writes and `faceNodes` reads only through it.
- The flip deleted the WebGL renderer, patches, smooth water, the dome sky and the WebGL clouds and precipitation in one change; Git holds them (`c131234`).
- Particles and the brush keep three's own materials for now; `CubeStage` puts a hemisphere and a directional light in the scene for them alone.
- Why far risers were dark (measured on the 8 km replay with the albedo set to grey): the light itself, not the colour. Backlit, a sunlit top took 3 to 4 times the light of a shaded riser, which is right for a real wall (a shaded wall gets half the sky and the bounce, about a fifth of what a sunlit flat gets). Removing the corner occlusion, giving risers the top's sky, snow on risers, a local bounce or the per-cube tone each moved the picture by a few levels at most; tone mapping and aerial perspective treat tops and risers alike. The carpet comes from the coarse levels cutting gentle slopes into steps one cube high, each with a riser in shade.
- Land light rather than finer far levels or smooth normals: a coarse cube stands for the voxels within it, and seen from where they fall within a pixel, a stair is the slope's own light. The faces keep their geometry, edges, corner occlusion and a quarter of their own light, so steps still read as cubes; ground steeper than 45° keeps its own light, as the engine keeps rock faces there. The heights are the level's own tops, kept in a ring per level that the chunks fill, so the slope needs nothing from the engine and has no seams at chunk edges.
- The snow bounce follows from the bounce rule above: a face in shade under snow sees snow in front of it.

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
- A `Vector4` starts with w = 1; a uniform array of places that means "held" in w must be filled with zeros.
- Whether the backend is WebGPU is known only after `renderer.init()`; a world may load before that. Decide backend-dependent node graphs inside a `Fn` (its only argument is the node builder when it is called without inputs: `builder.renderer.backend.isWebGPUBackend`), not when the stage is made.
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
- Snow, tops only: risers of slopes snow lies on (up to about 45°) would need snow too, or near snowy ground shows rows of bare one-voxel risers as pins.

Lines added to shared files:

- `@game/src/index.ts`: import of `createCubeScene` and `cubeWorlds`; `const scene = createCubeScene(canvas, showError)` in `compose()`; `cubeWorlds(window.derivean)` passed to the session; `scene.covered = map.open;` before `session.tick` in `draw` (asked for by the map stream); `levels: scene.levelCounts()`, `backend`, `draw: scene.timings()` in `metrics.paint`; `chunksByLevel`, `backend`, `drawMs` in `measure`. The rest of `index.ts` besides the map's lines is this stream's.
- `@game/src/capture/service/CaptureDriver.ts`: `|| frame.cubes?.added.length` in the arrival test; the `brush` phase.
- `@game/main/index.mjs`: `loadURL(renderer.href)` replaces `loadFile`, and the sender check compares against the same address. `DERIVEAN_RENDERER` is gone.
- `docs/architecture.md`, Display: the Light row of the look table gained two sentences (the land light for coarse cubes, the snow bounce); the Shadows row names the clouds' shadows; the paragraph after the table says how the scene, clouds, cloud noise, rain and snow are drawn and that the WebGL 2 fallback draws no clouds.

The M key and `map/**` are the map stream's. The window title stays "DeRivean · Fable".

## 7. How to build, run and verify

- Build: `npm run build` in `@game` (Vite; it does not type-check). Types: `npx tsc --noEmit` in `@game`.
- Real app: from the worktree root, `DERIVEAN_CAPTURE=<plan.json> @game/node_modules/.bin/electron @game`. It uses `.out/addon/derivean.node` as built by the engine stream. Engine start takes 2 to 8 minutes under load. Plans: `@game/capture/look.json` (37 shots, the look's judge), `pair-cubes.json` (ten shots at the pinned land spot), `cubes-live.json` (the pinned spot from the ground, 300 m, 2 km and 8 km at hours 12 and 16, digging and placing, six stills one second of flight apart, a boost flight). Shots and records land in the plan's `out`.
- Capture shots may `brush: { frames, place?, radius?, seconds? }`: after settling, hold dig (or place basalt) along the view for that many frames and shoot `seconds` after the first stroke. They may `turn: { seconds, radiansPerSecond }`: after settling, keep turning and shoot while still turning (not with `cruise` or `brush`). `clouds: false` hides clouds, their shadows and precipitation. `clouds-live.json`: the pinned spot at hours 12 and 16 from the ground (looking up, turning), at 300 m turning, 2 km and 8 km; boost flights with and without clouds; a shot that seeks falling snow.
- Timing a pass truthfully: in a scratch build, a `__bench(n)` on `globalThis` that draws `n` frames back to back with the thing off and on, waiting on `device.queue.onSubmittedWorkDone()`, called by the replay's per-shot `bench` (`replay-main.mjs`). Shots there may also set `look` (a debug variant through `globalThis.__look`) and `clouds`.
- Replay, the fast loop (about a minute, no engine): tooling in `.out/renderer-replay/` (scratch, untracked). Record once with `ELECTRON_RUN_AS_NODE=1 @game/node_modules/.bin/electron .out/renderer-replay/record2.cjs ...` (see `spec-stills.json`; about 18 minutes), then `REPLAY_ROOT=<worktree> REPLAY_DATA=.out/renderer-replay/rec-stills REPLAY_PLAN=<plan.json> @game/node_modules/.bin/electron .out/renderer-replay`. A plan lists shots `{ name, segment, yaw, pitch, plain?, sun? }` (segment 0 ground, 1 at 300 m, 2 at 2 km, 3 at 8 km, 4 at 30 km; `sun` a unit vector towards the sun overrides the recorded one) and `out`. `rec-stills` was recorded at 11:22 on 26 Nov at spawn `[-2521902, -4671906]` (seed 42, 4.6 Gyr, noon; about 23 minutes at load 95); it is stale once the engine's quads change. The recorder prints its log only when it exits.
- Frame statistics are not trustworthy above load about 40; say the load with every number. Close every instance you start; other streams run instances too, so check a process's parent and working directory before ending it.
- Good looks like: the same view in cubes at every distance; walls, ledges and crests crisp; strata as bands on far cliffs; cubes told apart by light and corner occlusion under overcast; shadows that give ranges their mass at a low sun; no shimmer where risers are near a pixel; no holes at level boundaries; aerial perspective to the limit of view; water as flat sheets with a shore; snow above a snow line. Compare with `docs/reference-landscapes.md`.
- Captures of this run: `.out/playwright/renderer/` (`shadow-*`, `lowsun-*`, `snowmean-*`, `flip-*`, `clouds-*`, `bench-*` from replays of the stale recording; `live-*` from the real app before the clouds, `clouds-live-*` with them, `land-before-*` and `land-after-*` for the land light, `coldrange-*` and `coldrange-lowsun-*` for the cold range). A capture's hour is local solar time: pick it from the sun the shot record reports. WebGL baseline before the flip: `.out/playwright/look/` and `look/pair-smooth/`.

## 8. Open questions

- Is the GPU within budget on weaker machines? Frames hold 8.3 ms here with up to 6.9 M triangles unculled plus up to one shadow cascade a frame; the timestamp query is not trusted. Splitting a batch by quadrant or indirect multi-draw are the candidates if not.
- Shadow map memory: three 4096² and one 2048² float depth maps (about 210 MB with their colour targets). Generous by the owner's rule; pull down only if a machine cannot hold it.
- Risers of a one-cube step under a covered top: bed or cover (section 6)?
- The cut between levels and the shadow cascades move in steps as the eye moves. Whether that reads as popping has not been watched in motion.
- The `bands` texture is RGBA32F with linear filtering, which needs the `float32-filterable` feature; fine on Apple, unchecked elsewhere. Half floats would round heights to 8 m at altitude.
- Should risers of cubes that stand for sloping ground take some of the snow? The contract says tops only (section 6).
- The land light's ring holds a level's box and band at the default reach (128); at a larger `cubeReach` the outer squares share places with the inner ones and the farther ones read as not held (faces keep their own light there). Size the ring from `reach` if reach is raised.
- The WebGL 2 fallback still keeps the level heights on the CPU (about 13 MB) though it never reads them.
