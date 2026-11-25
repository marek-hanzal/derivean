import "./launcher/ui/launcher.css";

import type { Frame } from "../contract/GameBridge";
import { createWorldControls } from "./control/service/WorldControls";
import { createFrameMetrics } from "./diagnostics/service/FrameMetrics";
import { createWorldScene } from "./scene/service/WorldScene";

function select<T extends HTMLElement>(selector: string) {
  const element = document.querySelector<T>(selector);
  if (!element) throw new Error(`Missing interface element ${selector}`);
  return element;
}

const game = select("#game");
const canvas = select<HTMLCanvasElement>("#world");
const menu = select("#menu");
const hud = select("#hud");
const play = select<HTMLButtonElement>("#play");
const playLabel = select("#play-label");
const message = select("#message");
const output = select<HTMLOutputElement>("#metrics");
const materialName = select("#material");
const stock = select("#stock");
const crosshair = select(".crosshair");
const brushSize = select("#brush-size");
const sphere = select("#sphere");
const cube = select("#cube");

function showError(error: unknown) {
  if (document.pointerLockElement) document.exitPointerLock();
  play.disabled = true;
  playLabel.textContent = "World unavailable";
  message.textContent = error instanceof Error ? error.message : String(error);
  message.hidden = false;
  game.dataset.mode = "menu";
  menu.hidden = false;
  hud.hidden = true;
}

async function launch() {
  const world = await window.derivean.startWorld();
  const visual = createWorldScene(canvas, world);
  const metrics = createFrameMetrics(output, world);
  const cellsPerMeter = Math.round(1 / world.voxelSize);
  select("#voxel-scale").textContent = `${cellsPerMeter}³ VOXELS / m³`;
  let entered = false;
  let ready = false;
  let failed = false;
  const controls = createWorldControls(canvas, world, (locked) => {
    if (failed) return;
    if (locked) entered = true;
    hud.hidden = !locked;
    menu.hidden = locked;
    game.dataset.mode = locked ? "play" : "menu";
    if (!locked) playLabel.textContent = entered ? "Resume world" : "Enter world";
  });
  play.addEventListener("click", () => {
    try {
      message.hidden = true;
      const request = controls.lock();
      void request?.catch((error: unknown) => {
        message.textContent = error instanceof Error ? error.message : String(error);
        message.hidden = false;
      });
    } catch (error) {
      showError(error);
    }
  });
  window.addEventListener("keydown", (event) => {
    if (event.code !== "F3" || event.repeat || !controls.locked()) return;
    event.preventDefault();
    output.hidden = !output.hidden;
  });

  let frame: Frame | undefined;
  let pending = false;
  let lastDraw = performance.now();
  let lastAdvance = lastDraw;

  function draw(time: number) {
    if (failed) return;
    requestAnimationFrame(draw);
    const dt = time - lastDraw;
    lastDraw = time;
    const active = controls.locked();
    if (active && dt > 0) metrics.frame(dt);

    if (!pending && (active || !frame || !frame.streamingReady)) {
      const elapsed = Math.max(0, (time - lastAdvance) / 1000);
      const command = controls.command(elapsed, time);
      pending = true;
      lastAdvance = time;
      const dispatch = performance.now();
      void window.derivean.advance(command).then((next) => {
        frame = next;
        visual.apply(next);
        if (command.active) metrics.advance(next, performance.now() - dispatch);
        if (!ready && next.streamingReady) {
          ready = true;
          play.disabled = false;
          playLabel.textContent = "Enter world";
          game.dataset.mode = "menu";
        }
      }).catch((error: unknown) => {
        failed = true;
        showError(error);
      }).finally(() => { pending = false; });
    } else if (!active && !pending) {
      lastAdvance = time;
    }

    const target = active ? frame?.target : undefined;
    const material = target ? world.materials[target.material] : undefined;
    visual.target(target?.voxel, controls.shape, controls.radius);
    visual.draw(active, entered, controls.yaw, controls.pitch, Math.max(0, dt) / 1000, time / 1000);
    if (!active || !frame) return;
    crosshair.dataset.target = String(target !== null && target !== undefined);
    stock.textContent = String(frame.buildStock);
    materialName.textContent = material ? `${material.name} · ${material.density} kg/m³ · ${material.matter === "granular" ? `${material.angleOfRepose}° repose` : `${material.support} kg/face`}` : "—";
    const diameter = (2 * controls.radius - Number(controls.shape === "cube")) * world.voxelSize;
    brushSize.textContent = `${diameter.toFixed(2)} m`;
    sphere.dataset.selected = String(controls.shape === "sphere");
    cube.dataset.selected = String(controls.shape === "cube");
    metrics.paint(time, frame, visual.renderStats.calls, visual.renderStats.triangles, controls.yaw, controls.pitch);
  }

  requestAnimationFrame(draw);
}

select("#quit").addEventListener("click", () => { void window.derivean.quit(); });
void launch().catch(showError);
