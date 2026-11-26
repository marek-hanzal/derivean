# Hand-over: in-game map (`M`)

Describes the present state of the `map` workstream. Rewrite it when the state changes; it is not a diary. Nothing here is committed yet; the lead commits.

## 1. Goal

`M` opens a map of the simulated world over the game and `M` closes it (`Esc` also closes). The map is drawn from data the C++ simulation gives, never from what the renderer holds: ground elevation, sea, lakes and streams, glacier ice, climate warmth, loose cover and rock, and the level of detail each pixel came from. It shows the finest data already built and the planet elsewhere, never waits for tiles, and must not cost the game frames. On it: north up, the player's arrow with heading, scale bar, cursor readout, wheel zoom about the cursor from the whole planet to a 1 km span, drag to pan, `C` to centre on the player, seed, age and calendar date, a hatch over ground known only at planet scale, and Shift+click to travel.

## 2. State now

Works, in the real game, today:

- **Simulation**: `sim::chartOf(const World&, ChartRequest)` in `@simulation/src/chart/` reads a square at a raster size. Per pixel it is `surfaceAt` at the pixel centre from the finest finished level the pitch can show (a pixel spans at most 4 cells of a level; never finer than Site), and the planet through `planetPoint` where no tile is finished. It never builds or blocks. `terra chart` writes the fields as a quick PNG.
- **Bridge**: `Session.chart(request)` in `@napi/src/chart.cpp` returns a promise at once and reads on libuv's pool (`Napi::AsyncWorker`). All nine rasters are views of one `ArrayBuffer`, allocated on the session thread and filled off it. `ChartGate` makes `restart`, `abandon` and the session's destruction wait for charts still reading the world. Relayed by `engine-worker.cjs`, `engine.mjs`, `main/index.mjs`, `preload/index.cjs` as `window.derivean.chart`.
- **Overlay**: `@game/src/map/`. It draws on **its own canvas with its own WebGL 2 context** (`service/ChartSurface.ts`, plain WebGL 2, no Three.js) and does not touch the game's renderer, so the move of the game to WebGPU does not affect it. Text, scale bar, legend and the player's arrow are DOM.
- **Keys and pointer lock** work with real key and mouse events: M opens (pointer freed, HUD and mode untouched, world controls inert, world clock keeps running), M closes and recaptures the pointer, Esc closes and recaptures the pointer when the key comes up, wheel zoom keeps the point under the cursor, drag pans, C centres, Shift+click teleports exactly there, W walks again afterwards.

How verified:

- `@simulation/test/Chart.cpp` (`testChart`): equals `surfaceAt` at pixel centres at site, local, region pitch and at pitch = cell; planet pixels equal the planet read; magnified streams and shores checked against the tiles' own cells; quarters of a square equal the square; a square across the planet's seam; same request twice gives identical rasters; tile count unchanged. Passes when run alone. `mise -C @simulation run test` stops earlier in another stream's `testClimate`, so the suite does not reach it; run it alone as in section 7.
- `@napi/test/chart.test.cjs`: promise returns before the chart is read, shape of the answer, `unless`, positions to ±1e12 m, bad requests refused, restart with 12 charts in flight, and a worker thread terminated with 12 charts in flight (child process `chart-teardown.cjs`). All five passed; the last one was run alone after its fix (it caught a real abort, fixed in `ChartWork::OnOK`).
- Captures under `.out/playwright/map/` from `@game/capture/map.json`: `map-5km.png`, `map-200km.png`, `map-1000km.png` (refined tiles against hatched planet-scale ground), `map-planet.png`, `map-closed.png` (the game after closing), each with a `.json` record.
- Real input, driven through `webContents.sendInputEvent` by a scratch script (not in the repository): `keys-map-open.png`, `keys-map-zoomed.png`, `keys-map-travelled.png`, `keys-map-closed.png`, `keys.json`, `keys-lock.json` in the same folder. 12 of 12 and 5 of 5 checks passed. These runs need the window to hold the focus; other windows on the machine taking it makes them fail for reasons that are not the map's.
- Frame times, 20 s boosted flight at 700 m/s over the same path on a 120 Hz display, map closed against open at a 5 km span (`@game/capture/map-flight.json`, records `flight-*.json`; legs closed, open, closed, open after a warm-up leg): p50 8.30 ms in all four; p95 9.0, 9.0, 9.0, 9.1; p99 9.3, 9.3, 9.3, 9.5; worst frame 9.4, 16.6, 9.4, 17.5; frames in 20 s 2401, 2400, 2401, 2394. The open legs brought 26 and 65 charts on screen. So the percentiles match and the map costs at most a handful of single missed refreshes in 20 s. The machine was under heavy load from other work throughout; the closed warm-up leg had a 25 ms frame of its own.

Broken or weak:

- Nothing known broken in the map. An independent review found no blocker; its findings are fixed except those listed under 8.
- The simulation's levels disagree with each other (a lake at local level that is dry land at site level, a step in height at the edge of a finer tile). The map shows what is there, so tile-shaped rectangles appear at mixed levels. Relief and contours are drawn flat across such an edge.

## 3. In progress

Nothing half-done.

## 4. Next steps

1. Run `@game/capture/map-flight.json` again on a quiet machine; the figures in section 2 were taken at a load average of 30–50.
2. Ask the renderer's owner to skip drawing the 3D scene while `map.open` is true: the map is opaque, the scene under it costs the whole frame budget, and that is what leaves no room for the map's own work.
3. If the simulation gains a planet fallback and an eased surface of its own (the engine has `surfaceOrPlanet` and `surfaceEased` in `@engine/src/nature/type/SurfaceReader.hpp`), call them from `chartOf` and drop its own planet read and the shader's seam flattening.
4. Optional: a graticule with coordinates; place names once the world has any.

## 5. Decisions and why

- **Unwrapped positions.** The planet repeats every `WorldInfo.planetSpan`, but `planetPoint`'s warp (`@simulation/src/landscape/fn/Forcing.cpp`) has no period, so the landscape at `x + span` is not the landscape at `x`, and tiles are keyed by unwrapped position. A chart therefore samples where it is asked; wrapping positions would tear the picture or show tiles where there are none. Panning is endless and the repeat is close, not exact.
- **Chart off the session thread through `Napi::AsyncWorker`**, not the simulation's or the engine's pool: those run tile builds and engine tasks, and the promise has to settle on the session thread anyway.
- **One buffer for all rasters**: the worker transfers a single `ArrayBuffer`, and the views survive the two hops.
- **Coarse chart, then nine 256² panes side by side**: each message stays under 2 MB, the engine reads panes in parallel, and one pane a frame is handed to the GPU. A view that moves too fast for a sharp chart to stay valid 1.5 s keeps to coarse ones.
- **Spans from a ladder and centres from the pixel lattice of the span**, so a view that pans a little samples the same world points and its streams and shores do not shimmer.
- **Water drawn to be read**: a pixel wider than a cell takes the largest flow under it; a pixel narrower than a cell lies on a stream only along the path from cell centre to cell centre, and standing water reaches into a dry neighbour as far as the ground lets it. The shader then puts the shore where the smoothed ground meets the water level.
- **Stream threshold and relief exaggeration are read off each chart** (`fn/chartLook.ts`), because flows and slopes differ by orders of magnitude between levels.
- **No rivers from the planet level**: a planet cell knows the river that leaves it, not where in 65 km it runs.
- **Esc recaptures the pointer on key up.** Chromium ends a pointer lock that is in force when Escape comes up; locking on key down sent the player to the menu.
- **Moving views are drawn at one pixel per CSS pixel** into a framebuffer and stretched, and sharply once at rest: a quarter of the fill on a 2× display, without resizing the canvas.
- **`steer` wraps the pilot** so the session keeps simulating under the map with a command that only waits; this needed no change to the session.

## 6. Contracts

`GameBridge.chart(request: ChartRequest): Promise<Chart | null>` in `@game/contract/GameBridge.ts`. Request: `x`, `z`, `span` (metres), `size` (pixels a side, at most 1024), optional `unless` (the `tiles` of a chart already held; the answer is `null` when nothing was built since). Answer: `origin` (north-west corner), `pitch`, `size`, `tiles`, `finest`, and rasters of `size²` entries, row-major with Z as the row: `ground`, `water` (NaN when dry), `discharge`, `ice`, `warmth`, `cover` (Float32Array), `bedrock`, `waters` (0 dry, 1 sea, 2 lake, 3 stream), `level` (0 planet, 1 region, 2 local, 3 site) (Uint8Array). `WorldInfo.planetSpan` was added for the zoom limit.

Lines added to files other streams own:

- `@simulation/tool/main.cpp`: `if (command == "chart") return chartCommand(arguments);`; `@simulation/tool/Arguments.hpp`: `int chartCommand(const Arguments& arguments);`; `@simulation/test/Main.cpp`: `void testChart();` and `testChart();`.
- `@napi/src/addon.cpp`: `#include "chart.hpp"`; `InstanceMethod("chart", &Session::chart),`; member `ChartGate charts_;` after `game_`; `charts_.wait();` before the game is reset in `start` (the engine's owner added the same in `abandon`); `output["planetSpan"] = sim::PlanetSpan;`; the one-line `chart` method.
- `@game/contract/GameBridge.ts`: `planetSpan`, `ChartRequest`, `Chart`, `chart(...)`.
- `@game/preload/index.cjs`: `chart: (request) => ipcRenderer.invoke("derivean:chart", request),`. `@game/main/engine.mjs`: `chart: (request) => send("chart", request),`. `@game/main/index.mjs`: `handle("derivean:chart", (request) => engine.chart(request));`. `@game/main/engine-worker.cjs`: the `kind === "chart"` branch. `@game/main/capture.mjs`: the `shot.map` check.
- `@game/index.html`: `<span><kbd>M</kbd> Map</span>` in the HUD legend; `<section id="map" class="map layer" data-ui="WorldMap" aria-label="Map" hidden></section>`.
- `@game/src/index.ts`: `import "./map/ui/map.css";`; `import { createWorldMap } ...`; `if (map.open) return;` in the pointer-lock callback; `const map = createWorldMap(select(document, "#map"), window.derivean, { playing: () => controls.locked(), lock: () => controls.lock() });`; `map.paint(world, frame, pilot.yaw, time);` in `draw`; `map` in `compose()`'s return and in `boot()`'s destructuring; `session.steer(map.steer(controls.pilot));`; in the capture host `setMap`, `mapSettled`, and `map: map.describe()` in `measure`.
- `@game/src/capture/type/CapturePlan.ts`: `map?: { span: number }`. `@game/src/capture/service/CaptureDriver.ts`: `setMap` and `mapSettled` in `CaptureHost`, `host.setMap(current.map?.span);`, `&& host.mapSettled()` in the settle condition.
- `@game/src/diagnostics/fn/summary.ts`: `p50`.
- `README.md`: the `M` row. `docs/architecture.md`: section "Map". `docs/simulation.md`: the `chartOf` paragraph.

`WorldControls.ts` is untouched: the map listens to keys itself and relies on the controls ignoring everything while the pointer is free. What the overlay needs from the renderer's owner is only those lines in `index.ts` and the `#map` section; `map/**` imports `calendar/fn/readClock`, `calendar/constant/rates`, `control/type/Pilot`, `hud/fn/worldName`, `units/fn/length`, `dom/fn/select`.

## 7. How to build, run and verify

- Build: `mise -C @simulation run build`, `mise -C @napi run build`, `mise -C @game run build`. Type-check: `cd @game && npx tsc --noEmit`.
- Simulation picture: `.out/simulation/build/macosx/arm64/release/terra chart --seed 42 --age 4.6e9 --span 16777216 --out .out/terra/chart-planet.png`, and `--at landing --span 20000 --out .out/terra/chart-landing-20km.png`; `--refine 1..3` first builds the tiles under the square.
- Simulation test alone, while the suite stops earlier: compile a `main` that calls `testChart()` with `@simulation/test/Chart.cpp`, `@simulation/test/LandscapeFixture.cpp` and `.out/simulation/build/macosx/arm64/release/libderivean-simulation.a` (`clang++ -std=c++20 -O2 -I@simulation/src -I@simulation/test`).
- Bridge test: `cd @napi && node --test test/chart.test.cjs` (several minutes: it evolves three planets).
- Screenshots: `DERIVEAN_CAPTURE=@game/capture/map.json mise -C @game run play`; frame times: the same with `@game/capture/map-flight.json` (its start position is a fixed place of seed 42; any place works).
- By hand: `mise run play`, enter the world, press `M`.

## 8. Open questions

- `planetPoint` is not periodic over the planet's span, so the world is not an exact torus (decision above). Its owner could give the warp a period.
- `unless` compares the atlas's total of finished tiles, of every level and anywhere. While tiles stream, an open map reads its nine panes again every 1.5 s even when nothing in view changed. A per-level count in `Atlas` would let a chart look only at the levels it reads.
- The snow line and the bare-rock line of the map (`constant/map.ts`) are the map's own reading of annual climate and cover. The world's snow on a given day comes from the atmosphere and is not on the map.
- Rivers read from a coarser level than the pitch are straight paths between cell centres; they sharpen when finer tiles exist.
- Not verified: other platforms than macOS on Apple silicon; a display at 1× pixel ratio; GPUs slower than an M3 Max; sanitizer runs of the restart and teardown tests.
