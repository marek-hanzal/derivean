import { ACESFilmicToneMapping, PerspectiveCamera, Scene, WebGPURenderer } from "three/webgpu";
import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import type { WeatherSought } from "../../clouds/fn/findWeather";
import { Exposure } from "../../sky/constant/air";
import { createCubeStage } from "./CubeStage";
import type { createWorldScene } from "./WorldScene";

// The WebGPU renderer and its camera, which outlive worlds; `load` swaps the stage. Depth is reversed in a float
// buffer, because one view holds cubes 6 cm from the eye and mountains hundreds of kilometres away. Nothing is
// drawn until the device is there; `backend` then says what three.js got, WebGPU or its WebGL 2 fallback. A
// device that cannot be had is reported through `failed`.
export function createCubeScene(canvas: HTMLCanvasElement, failed: (error: unknown) => void): ReturnType<typeof createWorldScene> {
  const renderer = new WebGPURenderer({
    canvas, antialias: true, powerPreference: "high-performance", reversedDepthBuffer: true, trackTimestamp: true,
  });
  renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5));
  renderer.toneMapping = ACESFilmicToneMapping;
  renderer.toneMappingExposure = Exposure;

  const scene = new Scene();
  scene.matrixAutoUpdate = false;
  const camera = new PerspectiveCamera(62, 1, 0.055, 1000);
  camera.rotation.order = "YXZ";
  const renderStats = { calls: 0, triangles: 0, points: 0, lines: 0, frame: 0 };
  let stage: ReturnType<typeof createCubeStage> | undefined;
  let backend = "starting";
  let cpuMs = 0;
  let gpuMs: number | null = null;
  let drawn = 0;
  let plain = false;
  let covered = false;
  let width = 0;
  let height = 0;

  void renderer.init().then(() => {
    backend = (renderer.backend as { isWebGPUBackend?: boolean }).isWebGPUBackend ? "WebGPU" : "WebGL 2 (three.js fallback)";
  }).catch(failed);

  return {
    get backend() { return backend; },
    load(world: WorldInfo) {
      stage?.dispose();
      stage = createCubeStage(scene, camera, world);
      stage.shade(plain);
    },
    // Leaves the canvas blank: the launcher is what shows until the next world loads.
    unload() {
      stage?.dispose();
      stage = undefined;
      if (backend !== "starting") renderer.clear();
    },
    // True while something opaque lies over the canvas, as the map does: the world is then kept but not drawn.
    get covered() { return covered; },
    set covered(value: boolean) { covered = value; },
    // Plain relief shading; the choice outlives the world it was made in.
    get plain() { return plain; },
    set plain(value: boolean) {
      plain = value;
      stage?.shade(value);
    },
    apply(frame: Frame) { stage?.apply(frame); },
    target(point: Point | undefined, shape: BrushShape, radius: number) { stage?.target(point, shape, radius); },
    // `entered` is false until the player first takes control of this world.
    draw(entered: boolean, yaw: number, pitch: number, dt: number, time: number) {
      if (!stage || covered || backend === "starting") return;
      if (canvas.clientWidth !== width || canvas.clientHeight !== height) {
        width = canvas.clientWidth;
        height = canvas.clientHeight;
        renderer.setSize(width, height, false);
        camera.aspect = width / Math.max(1, height);
        camera.updateProjectionMatrix();
      }
      stage.pose(entered, yaw, pitch, dt, time);
      renderer.toneMappingExposure += (stage.exposure - renderer.toneMappingExposure) * Math.min(1, dt * 2);
      const began = performance.now();
      renderer.render(scene, camera);
      cpuMs += (performance.now() - began - cpuMs) * 0.1;
      renderStats.calls = renderer.info.render.drawCalls;
      renderStats.triangles = renderer.info.render.triangles;
      // The GPU answers a few frames late, and not at all where it cannot time itself.
      if (++drawn % 8 === 0) {
        void renderer.resolveTimestampsAsync("render").then((taken) => {
          if (typeof taken === "number" && taken > 0) gpuMs = gpuMs === null ? taken : gpuMs + (taken - gpuMs) * 0.2;
        });
      }
    },
    // Clouds are not drawn by this renderer yet.
    veil() {},
    findWeather: (sought: WeatherSought) => stage?.findWeather(sought),
    patchCounts: () => stage?.counts() ?? [],
    openWaterCells: () => 0,
    cloudCost: () => 0,
    timings: () => ({ cpuMs, gpuMs }),
    renderStats,
  };
}
