# Hand-over: in-game map (`M`)

Describes the present state of the `map` workstream. Rewrite it when the state changes; it is not a diary. Nothing here is committed yet; the lead commits.

## 1. Goal

`M` opens a map of the simulated world over the game and `M` closes it (`Esc` also closes). The map is drawn from data the C++ simulation gives, never from what the renderer holds: ground elevation, sea, lakes and streams, glacier ice, climate warmth, loose cover and rock, and the level of detail each pixel came from. It shows the finest data already built and the planet elsewhere, never waits for tiles, and must not cost the game frames. On it: north up, the player's arrow with heading, scale bar, cursor readout, wheel zoom about the cursor from the whole planet to a 1 km span, drag to pan, `C` to centre on the player, seed, age and calendar date, a hatch over ground known only at planet scale, and Shift+click to travel.

## 2. State now

Works, in the real game, today:

- **Simulation**: `sim::chartOf(const World&, ChartRequest)` in `@simulation/src/chart/` reads a square at a raster size. Per pixel it is `readSurface` at the pixel centre from the finest finished level the pitch can show (a pixel spans at most 4 cells of a level, down to Ground; where it reads Ground, whether water stands is the site level's say), and the planet through `planetPoint` where no tile is finished. It never builds or blocks, and it reads only the planet and finished tiles. `terra chart` writes the fields as a quick PNG.
- **Bridge**: `Session.chart(request)` in `@napi/src/chart.cpp` returns a promise at once and reads on libuv's pool (`Napi::AsyncWorker`). All nine rasters are views of one `ArrayBuffer`, allocated on the session thread and filled off it. `ChartGate` makes `restart`, `abandon` and the session's destruction wait for charts still reading the world. Relayed by `engine-worker.cjs`, `engine.mjs`, `main/index.mjs`, `preload/index.cjs` as `window.derivean.chart`.
- **Overlay**: `@game/src/map/`. It draws on **its own canvas with the scene's WebGPU device** (`service/ChartSurface.ts`, plain WebGPU, no Three.js; the device from `WorldView.device`), and does not otherwise touch the game's renderer. A chart's eight rasters go up as two `rgba32float` sheets: relief (ground, discharge, level code, water kind) for the 4×4 B-spline and climate (water, ice, warmth, cover) for the 2×2 blend, 16 + 4 texel reads a pixel instead of 69 (DREV-NEON-056). A device that refuses the shader or a pipeline shows nothing and the map closes. Text, scale bar, legend and the player's arrow are DOM.
- **Keys and pointer lock** worked with real key and mouse events when last confirmed (01:40, before the last review's fixes): M opens (pointer freed, HUD and mode untouched, world controls inert, world clock keeps running), M closes and recaptures the pointer, Esc closes and recaptures the pointer when the key comes up, wheel zoom keeps the point under the cursor, drag pans, C centres, Shift+click teleports exactly there, W walks again afterwards.

How verified:

- `@simulation/test/Chart.cpp` and `ChartWater.cpp` (`testChart`, helpers in `ChartExpect.hpp`): equals `readSurface` at pixel centres at site, local, region pitch and at pitch = cell; planet pixels equal the planet read; streams and shores of pixels narrower than their cells checked against the tiles' own cells; quarters of a square equal the square; a square across the planet's seam; same request twice gives identical rasters; tile count unchanged. Passes when run alone. `mise -C @simulation run test` stops earlier in another stream's `testClimate`, so the suite does not reach it; run it alone as in section 7.
- `@napi/test/chart.test.cjs`: promise returns before the chart is read, shape of the answer, `unless`, positions to ±1e12 m, bad requests refused, restart with 12 charts in flight, and a worker thread terminated with 12 charts in flight (child process `chart-teardown.cjs`). All five passed; the last one was run alone after its fix (it caught a real abort, fixed in `ChartWork::OnOK`).
- Captures under `.out/playwright/map/` from `@game/capture/map.json`: `map-5km.png`, `map-200km.png`, `map-1000km.png` (refined tiles against hatched planet-scale ground), `map-planet.png`, `map-closed.png` (the game after closing), each with a `.json` record.
- Real input, driven through `webContents.sendInputEvent` by `.out/playwright/map/map-keys.mjs` (a driver outside the repository's sources; it loads the unmodified game): `keys-map-open.png`, `keys-map-zoomed.png`, `keys-map-travelled.png`, `keys-map-closed.png`, `keys.json`, `keys-lock.json` in the same folder. 12 of 12 and 5 of 5 checks passed on 26 Nov at 01:40, **before** the last review's fixes. Not repeated since: four runs on 26 Nov 09:07–10:02 (load 35–135) never reached "Enter world", so M was not confirmed on the current build. Run 1 waited 15 minutes with no page error. Run 3 showed why a run can hang: the launcher's planet clock froze while the window sat behind other windows, because a normal (non-capture) window is throttled and stops its frames when it is covered. Run 4 raised the window every 30 s and the frames kept going, but the planet was still generating after 1479 s with the game at about 500 % CPU. The capture run at 08:54 had its planet in about 8 minutes. Whether the planet now takes longer to build, or the load alone made it slow, is not established.
- Frame times, 20 s boosted flight at 528 m/s over the same path on a 120 Hz display, map closed against open at a 5 km span (`@game/capture/map-flight.json`, records `flight-*.json`; legs closed, open, closed, open after a warm-up leg), taken 26 Nov 09:03–09:06 from the `@game/dist` of 03:00 (map code as of `c131234`, without the last review's fixes) with the WebGL renderer, which skips the 3D scene while the map is open (`scene.covered`): p50 8.30 ms in all four; p95 8.9, 8.5, 8.8, 8.4; p99 10.4, 9.3, 9.3, 9.0; worst frame 17.3, 25.0, 16.8, 9.4; frames in 20 s 2378, 2399, 2392, 2401. The open legs brought 15 and 32 charts on screen (all from local tiles). So the open map is no dearer than the closed game; the single long frames occur in both. The machine was not quiet: load average 60 at the start of the legs, 135 at the worst, 99 at the end (other streams' builds and runs, and another game instance of another stream). The run before (load 30–50, scene still drawn under the map) is kept in `.out/playwright/map/prev/`.

Broken or weak:

- Nothing known broken in the map. Three independent reviews found no blocker; their findings are fixed except those listed under 8. The last one's three fixes (a "news" read is cancelled by a close or a world change that lands while its question is out; a world change redraws at once instead of leaving the old world on screen; a lost WebGL context is prevented, the surface is rebuilt when it comes back, and a surface that cannot be built leaves the map shut with the error in the console) A failure of a round that was dropped no longer closes a reopened map either: a round is one reading with one generation for its question and both its reads. These fixes are type-checked and were reviewed again. They are in the `@game/dist` built at 09:18, but they have not run in the game: no key run finished (above), and the context-loss path was never provoked.
- The simulation's levels disagree with each other (a lake at local level that is dry land at site level, a step in height at the edge of a finer tile). The map shows what is there, so tile-shaped rectangles appear at mixed levels. Relief and contours are drawn flat across such an edge.

## 3. In progress

Nothing half-done.

## 4. Next steps

1. Confirm M on the current build on a quiet machine (load under 10):
   - `cd @game && npx tsc --noEmit && npm run build`.
   - From the worktree root run `DERIVEAN_REPO=$PWD @game/node_modules/.bin/electron .out/playwright/map/map-keys.mjs`. Leave the window in front and do not touch the machine until it quits.
   - Expect 12 `ok` lines and a new `keys.json`.
   - By hand instead: `mise -C @game run play`, Enter world, press M (the map opens, the pointer is free, the HUD stays), M again (the map closes, the pointer is captured), then M and Esc (the map closes and the pointer is captured when Esc comes up).
   - If the launcher's "Generating the planet" clock stops counting, the window is covered: bring it to the front.
2. Run `@game/capture/map-flight.json` once more on a quiet machine and on the WebGPU renderer, now that the WebGL path is gone. The figures in section 2 were taken at a load average of 60–135 on WebGL.
3. Provoke a context loss once (`WEBGL_lose_context` on the map canvas from a driver script) to see the map shut and open again.
4. If the simulation gains a planet fallback and an eased surface of its own (the engine has `surfaceOrPlanet` and `surfaceEased` in `@engine/src/nature/type/SurfaceReader.hpp`), call them from `chartOf` and drop its own planet read and the shader's seam flattening.
5. Optional: a graticule with coordinates; place names once the world has any.

## 5. Decisions and why

- **The untouched world.** `surfaceAt` now lays the session's live changes over a sample (`overlayLive`), and that state belongs to the thread that drives time and edits. A chart is read on another thread, so it reads tiles through `readSurface` and leaves the overlay out: dug ground, live water and a sea that has moved are not on the map.
- **Unwrapped positions.** The planet repeats every `WorldInfo.planetSpan`, but `planetPoint`'s warp (`@simulation/src/landscape/fn/Forcing.cpp`) has no period, so the landscape at `x + span` is not the landscape at `x`, and tiles are keyed by unwrapped position. A chart therefore samples where it is asked; wrapping positions would tear the picture or show tiles where there are none. Panning is endless and the repeat is close, not exact.
- **Chart off the session thread through `Napi::AsyncWorker`**, not the simulation's or the engine's pool: those run tile builds and engine tasks, and the promise has to settle on the session thread anyway.
- **One buffer for all rasters**: the worker transfers a single `ArrayBuffer`, and the views survive the two hops.
- **Coarse chart, then nine 256² panes side by side**: each message stays under 2 MB, the engine reads panes in parallel, and one pane a frame is handed to the GPU. A view that moves too fast for a sharp chart to stay valid 1.5 s keeps to coarse ones.
- **Spans from a ladder and centres from the pixel lattice of the span**, so a view that pans a little samples the same world points and its streams and shores do not shimmer.
- **Water drawn to be read**: a pixel wider than a cell takes the largest flow under it; a pixel narrower than a cell lies on a stream only along the path from cell centre to cell centre, and standing water reaches into a dry neighbour as far as the ground lets it. The shader then puts the shore where the smoothed ground meets the water level.
- **Stream threshold and relief exaggeration are read off each chart** (`fn/chartLook.ts`), because flows and slopes differ by orders of magnitude between levels.
- **No rivers from the planet level**: a planet cell knows the river that leaves it, not where in 65 km it runs.
- **Esc recaptures the pointer on key up.** Chromium ends a pointer lock that is in force when Escape comes up; locking on key down sent the player to the menu.
- **Moving views are drawn at one pixel per CSS pixel** into a texture and stretched by a second pass with linear sampling, and sharply once at rest: a quarter of the fill on a 2× display, without resizing the canvas.
- **WebGPU only, as the scene** (owner, 27 Nov). The port keeps the WebGL 2 shader's arithmetic; a pixel outside the chart and its margin returns paper before any read. One difference stays: `fwidth` works on 2×2 quads counted from the top in WebGPU and from the bottom in WebGL, so at an odd drawn height the contours, shores and streams that use it fall one quad row differently (a pixel harness against the old shader: exact at even heights, 0.2 to 5 % of pixels moved at odd ones). Neither is more right.
- **`steer` wraps the pilot** (`fn/waitingPilot.ts`) so the session keeps simulating under the map with a command that only waits; this needed no change to the session.

## 6. Contracts

`GameBridge.chart(request: ChartRequest): Promise<Chart | null>` in `@game/contract/GameBridge.ts`. Request: `x`, `z`, `span` (metres), `size` (pixels a side, at most 1024), optional `unless` (the `tiles` of a chart already held; the answer is `null` when nothing was built since). Answer: `origin` (north-west corner), `pitch`, `size`, `tiles`, `finest`, and rasters of `size²` entries, row-major with Z as the row: `ground`, `water` (NaN when dry), `discharge`, `ice`, `warmth`, `cover` (Float32Array), `bedrock`, `waters` (0 dry, 1 sea, 2 lake, 3 stream), `level` (0 planet, 1 region, 2 local, 3 site, 4 ground) (Uint8Array). `WorldInfo.planetSpan` was added for the zoom limit.

Lines added to files other streams own:

- `@simulation/tool/main.cpp`: `if (command == "chart") return chartCommand(arguments);`; `@simulation/tool/Arguments.hpp`: `int chartCommand(const Arguments& arguments);`; `@simulation/test/Main.cpp`: `void testChart();` and `testChart();`.
- `@napi/src/addon.cpp`: `#include "chart.hpp"`; `InstanceMethod("chart", &Session::chart),`; member `ChartGate charts_;` after `game_`; `charts_.wait();` before the game is reset in `start` (the engine's owner added the same in `abandon`); `output["planetSpan"] = sim::PlanetSpan;`; the one-line `chart` method.
- `@game/contract/GameBridge.ts`: `planetSpan`, `ChartRequest`, `Chart`, `chart(...)`.
- `@game/preload/index.cjs`: `chart: (request) => ipcRenderer.invoke("derivean:chart", request),`. `@game/main/engine.mjs`: `chart: (request) => send("chart", request),`. `@game/main/index.mjs`: `handle("derivean:chart", (request) => engine.chart(request));`. `@game/main/engine-worker.cjs`: the `kind === "chart"` branch. `@game/main/capture.mjs`: the `shot.map` check.
- `@game/index.html`: `<span><kbd>M</kbd> Map</span>` in the HUD legend; `<section id="map" class="map layer" data-ui="WorldMap" aria-label="Map" hidden></section>`.
- `@game/src/index.ts`: the session's `failed: showError` became `failed(error) { map.hide(); showError(error); }`, so an engine failure does not leave the map over the error; `import "./map/ui/map.css";`; `import { createWorldMap } ...`; `if (map.open) return;` in the pointer-lock callback; `const map = createWorldMap(select(document, "#map"), window.derivean, { playing: () => controls.locked(), lock: () => controls.lock() });`; `map.paint(world, frame, pilot.yaw, time);` in `draw`; `scene.covered = map.open;` in `draw` (the renderer's owner's line: the scene is not drawn under the opaque map); `map` in `compose()`'s return and in `boot()`'s destructuring; `session.steer(map.steer(controls.pilot));`; in the capture host `setMap`, `mapSettled`, and `map: map.describe()` in `measure`.
- `@game/src/capture/type/CapturePlan.ts`: `map?: { span: number }`. `@game/src/capture/service/CaptureDriver.ts`: `setMap` and `mapSettled` in `CaptureHost`, `host.setMap(current.map?.span);`, `&& host.mapSettled()` in the settle condition.
- `@game/src/diagnostics/fn/summary.ts`: `p50`.
- `README.md`: the `M` row. `docs/architecture.md`: section "Map". `docs/simulation.md`: the `chartOf` paragraph.

`WorldControls.ts` is untouched: the map listens to keys itself and relies on the controls ignoring everything while the pointer is free. What the overlay needs from the renderer's owner is only those lines in `index.ts` and the `#map` section; `map/**` imports `calendar/fn/readClock`, `calendar/constant/rates`, `control/type/Pilot`, `hud/fn/worldName`, `units/fn/length`, `dom/fn/select`.

## 7. How to build, run and verify

- Build: `mise -C @simulation run build`, `mise -C @napi run build`, `mise -C @game run build`. Type-check: `cd @game && npx tsc --noEmit`.
- Simulation picture: `.out/simulation/build/macosx/arm64/release/terra chart --seed 42 --age 4.6e9 --span 16777216 --out .out/terra/chart-planet.png`, and `--at landing --span 20000 --out .out/terra/chart-landing-20km.png`; `--refine 1..3` first builds the tiles under the square.
- Simulation test alone, while the suite stops earlier: compile a `main` that calls `testChart()` with `@simulation/test/Chart.cpp`, `@simulation/test/ChartWater.cpp`, `@simulation/test/LandscapeFixture.cpp` and `.out/simulation/build/macosx/arm64/release/libderivean-simulation.a` (`clang++ -std=c++20 -O2 -I@simulation/src -I@simulation/test`).
- Bridge test: `cd @napi && node --test test/chart.test.cjs` (several minutes: it evolves three planets).
- Screenshots: `DERIVEAN_CAPTURE=@game/capture/map.json mise -C @game run play`; frame times: the same with `@game/capture/map-flight.json` (its start position is a fixed place of seed 42; any place works).
- By hand: `mise run play`, enter the world, press `M`.

## 8. Open questions

- Live changes are not on the map (decision above). Showing them needs a snapshot handed over by the session thread, or a reader the interventions' owner declares safe from another thread.
- `planetPoint` is not periodic over the planet's span, so the world is not an exact torus (decision above). Its owner could give the warp a period.
- `unless` compares the atlas's total of finished tiles, of every level and anywhere. While tiles stream, an open map reads its nine panes again every 1.5 s even when nothing in view changed. A per-level count in `Atlas` would let a chart look only at the levels it reads.
- The snow line and the bare-rock line of the map (`constant/map.ts`) are the map's own reading of annual climate and cover. The world's snow on a given day comes from the atmosphere and is not on the map.
- Rivers read from a coarser level than the pitch are straight paths between cell centres; they sharpen when finer tiles exist.
- Not verified: other platforms than macOS on Apple silicon; a display at 1× pixel ratio; GPUs slower than an M3 Max; sanitizer runs of the restart and teardown tests.
