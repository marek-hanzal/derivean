import "./shell/ui/shell.css";
import "./launcher/ui/launcher.css";
import "./hud/ui/hud.css";
import "./inspector/ui/inspector.css";
import "./capture/ui/capture.css";
import "./calendar/ui/clock.css";
import "./map/ui/map.css";

import type { WorldOptions } from "../contract/GameBridge";
import { createClockPanel } from "./calendar/service/ClockPanel";
import { createWorldClock } from "./calendar/service/WorldClock";
import { dataLevels } from "./capture/fn/dataLevels";
import { createCaptureDriver } from "./capture/service/CaptureDriver";
import { createWorldControls } from "./control/service/WorldControls";
import { createFrameMetrics } from "./diagnostics/service/FrameMetrics";
import { select } from "./dom/fn/select";
import { worldName } from "./hud/fn/worldName";
import { createGameHud } from "./hud/service/GameHud";
import { createInspector } from "./inspector/service/Inspector";
import { streamingProgress } from "./launcher/fn/streamingProgress";
import { createWorldLauncher } from "./launcher/service/WorldLauncher";
import { createWorldMap } from "./map/service/WorldMap";
import { createCubeScene } from "./scene/service/CubeScene";
import { createWorldScene } from "./scene/service/WorldScene";
import { cubeWorlds } from "./session/fn/cubeWorlds";
import { createWorldSession } from "./session/service/WorldSession";
import { formatAge } from "./units/fn/age";
import { metres } from "./units/fn/length";

const game = select(document, "#game");
const canvas = select<HTMLCanvasElement>(document, "#world");
const menu = select(document, "#menu");
const hudRoot = select(document, "#hud");
const play = select<HTMLButtonElement>(document, "#play");
const playLabel = select(document, "#play-label");
const message = select(document, "#message");
const output = select<HTMLOutputElement>(document, "#metrics");
const captureLine = select(document, "#capture-line");
const worldLabel = select(document, "#world-name");

let start: ((options?: WorldOptions) => void) | undefined;
let enterable = false;
let broken = false;
let inspectRequested = false;
let generating: { options: WorldOptions | undefined; since: number } | undefined;

const launcher = createWorldLauncher(menu, (options) => start?.(options));

// A world that failed to start can be asked for again: the play button turns into the retry. Without a renderer
// there is nothing to retry.
function showError(error: unknown) {
  enterable = false;
  broken = true;
  generating = undefined;
  playLabel.textContent = start ? "Try again" : "World unavailable";
  play.disabled = !start;
  message.textContent = (error instanceof Error ? error.message : String(error)) || "The world failed without saying why";
  message.hidden = false;
  launcher.progress("");
  launcher.forget();
  if (document.pointerLockElement) document.exitPointerLock();
  if (game.dataset.mode !== "capture") game.dataset.mode = "menu";
  void window.deriveanCapture?.finish(message.textContent);
}

// Everything that needs the GPU. Throws when the canvas cannot give a context. The page's address says which
// renderer draws: `renderer=webgpu` the one of cubes, anything else the WebGL one of patches.
function compose() {
  const cubes = new URLSearchParams(location.search).get("renderer") === "webgpu";
  const scene = cubes ? createCubeScene(canvas) : createWorldScene(canvas);
  const metrics = createFrameMetrics(output);
  const hud = createGameHud(hudRoot);
  const inspector = createInspector(select(document, "#inspector"), window.derivean);
  const controls = createWorldControls(canvas, (locked) => {
    if (game.dataset.mode === "capture") return;
    // The map frees the pointer without leaving the world.
    if (map.open) return;
    hudRoot.hidden = !locked;
    menu.hidden = locked;
    game.dataset.mode = locked ? "play" : enterable || broken ? "menu" : "boot";
    if (!locked && enterable) playLabel.textContent = session.visited ? "Resume world" : "Enter world";
  });
  const map = createWorldMap(select(document, "#map"), window.derivean, { playing: () => controls.locked(), lock: () => controls.lock() });
  const clock = createWorldClock();
  const clockPanel = createClockPanel(menu, hudRoot, clock);
  const session = createWorldSession(cubes ? cubeWorlds(window.derivean) : window.derivean, scene, metrics, clock, {
    starting(options) {
      enterable = false;
      broken = false;
      generating = { options, since: performance.now() };
      play.disabled = true;
      playLabel.textContent = "Preparing world";
      message.hidden = true;
      worldLabel.textContent = "";
      inspector.close();
      if (game.dataset.mode !== "capture") game.dataset.mode = "boot";
    },
    started(world) {
      generating = undefined;
      controls.configure(world);
      hud.world(world);
      worldLabel.textContent = worldName(world);
      select(document, "#voxel-scale").textContent = `${Math.round(1 / world.voxelSize)}³ VOXELS / m³`;
    },
    frame(frame) {
      launcher.progress(streamingProgress(frame));
      if (enterable || !frame.streamingReady) return;
      enterable = true;
      play.disabled = false;
      playLabel.textContent = "Enter world";
      if (game.dataset.mode === "boot") game.dataset.mode = "menu";
    },
    failed: showError,
  });
  start = session.start;

  play.addEventListener("click", () => {
    message.hidden = true;
    if (broken) return launcher.open();
    void Promise.resolve().then(() => controls.lock()).catch((error: unknown) => {
      message.textContent = error instanceof Error ? error.message : String(error);
      message.hidden = false;
    });
  });
  window.addEventListener("keydown", (event) => {
    if (event.repeat || !controls.locked()) return;
    if (event.code === "F3") {
      event.preventDefault();
      output.hidden = !output.hidden;
    }
    if (event.code === "KeyL") scene.plain = !scene.plain;
    if (event.code === "KeyI") inspectRequested = true;
    clockPanel.press(event.code);
  });

  function draw(time: number) {
    requestAnimationFrame(draw);
    if (generating) {
      const { options, since } = generating;
      const which = options ? ` · seed ${options.seed} · ${formatAge(options.age)}` : "";
      launcher.progress(`Generating the planet${which} · ${Math.max(0, Math.floor((time - since) / 1000))} s`);
    }
    const shown = session.tick(time);
    if (!shown) return;
    const { world, frame, active, pilot } = shown;
    clockPanel.paint(frame.calendar);
    map.paint(world, frame, pilot.yaw, time);
    if (!captureLine.hidden) {
      const [x, y, z] = frame.eye;
      captureLine.textContent = `${worldName(world)}   X ${metres(x)}  Y ${metres(y)}  Z ${metres(z)} m`;
    }
    if (!active) return;
    if (inspectRequested) {
      inspectRequested = false;
      if (inspector.open) inspector.close();
      else {
        const at = frame.target?.voxel.map((cell) => (cell + 0.5) * world.voxelSize) ?? frame.eye;
        void inspector.inspect(at[0], at[2]);
      }
    }
    hud.paint(world, frame, pilot);
    metrics.paint(time, frame, {
      calls: scene.renderStats.calls, triangles: scene.renderStats.triangles, patches: scene.patchCounts(),
      yaw: pilot.yaw, pitch: pilot.pitch, rate: clock.rate, cloudMs: scene.cloudCost(), openWaterCells: scene.openWaterCells(),
      backend: scene.backend, draw: scene.timings(),
    });
  }
  requestAnimationFrame(draw);
  return { scene, metrics, session, controls, clock, map };
}

async function boot() {
  const { scene, metrics, session, controls, clock, map } = compose();
  const capture = window.deriveanCapture;
  if (!capture) {
    session.steer(map.steer(controls.pilot));
    launcher.open();
    return;
  }
  const plan = await capture.plan();
  game.dataset.mode = "capture";
  captureLine.hidden = false;
  session.steer(createCaptureDriver(capture, plan, {
    clock,
    setPlain(plain) { scene.plain = plain; },
    setClouds: scene.veil,
    findWeather: scene.findWeather,
    startLap: metrics.startLap,
    setMap: (span) => span === undefined ? map.hide() : map.show(span),
    mapSettled: map.settled,
    measure: (lap) => ({
      frameMs: lap ? metrics.endLap() : metrics.frames(), roundTripMs: metrics.roundTrips(),
      drawCalls: scene.renderStats.calls, triangles: scene.renderStats.triangles, patchesByLevel: scene.patchCounts(),
      metresPerSecond: Number(metrics.speed.toFixed(1)), viewport: [canvas.clientWidth, canvas.clientHeight],
      cloudGpuMs: scene.cloudCost(), map: map.describe(), openWaterCells: scene.openWaterCells(), backend: scene.backend,
      drawMs: scene.timings(),
    }),
    survey: (x, z) => dataLevels(window.derivean, x, z),
  }));
  session.start(plan.world);
}

select(document, "#quit").addEventListener("click", () => { void window.derivean.quit(); });
void boot().catch(showError);
