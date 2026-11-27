import { ACESFilmicToneMapping, PerspectiveCamera, Scene, WebGPURenderer } from "three/webgpu";
import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import type { WeatherSought } from "../../clouds/fn/findWeather";
import { Exposure } from "../../sky/constant/air";
import type { WorldView } from "../type/WorldView";
import { createCubeStage } from "./CubeStage";

// The world on screen: the WebGPU renderer and its camera, which outlive worlds; `load` swaps the stage. Depth
// is reversed in a float buffer, because one view holds cubes 6 cm from the eye and mountains hundreds of
// kilometres away. The scene is drawn multisampled into the cloud pass's own target, so the canvas itself needs no
// antialiasing. The game draws with WebGPU alone: three.js would fall back to WebGL 2 where WebGPU cannot be had,
// and that is switched off (three 0.187 offers no option for it, so its `_getFallback` is cleared), and a renderer
// that still came up on anything else is refused. Nothing is drawn until the device is there; one that cannot be
// had is reported through `failed`.
export function createCubeScene(canvas: HTMLCanvasElement, failed: (error: unknown) => void): WorldView {
  const renderer = new WebGPURenderer({
    canvas, antialias: false, powerPreference: "high-performance", reversedDepthBuffer: true, trackTimestamp: true,
  });
  (renderer as unknown as { _getFallback: null })._getFallback = null;
  renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5));
  renderer.toneMapping = ACESFilmicToneMapping;
  renderer.toneMappingExposure = Exposure;

  const scene = new Scene();
  scene.matrixAutoUpdate = false;
  const camera = new PerspectiveCamera(62, 1, 0.055, 1000);
  camera.rotation.order = "YXZ";
  const renderStats = { calls: 0, triangles: 0 };
  let stage: ReturnType<typeof createCubeStage> | undefined;
  let backend = "starting";
  let cpuMs = 0;
  let gpuMs: number | null = null;
  let drawn = 0;
  let plain = false;
  let covered = false;
  let width = 0;
  let height = 0;
  // While a bench runs it alone draws, so the frames it times are not shared with the display's.
  let benching = false;

  void renderer.init().then(() => {
    if (!(renderer.backend as { isWebGPUBackend?: boolean }).isWebGPUBackend) throw new Error("not a WebGPU device");
    backend = "WebGPU";
  }).catch((error: unknown) => failed(new Error(`WebGPU is required: ${error instanceof Error ? error.message : String(error)}`)));

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
    get covered() { return covered; },
    set covered(value: boolean) { covered = value; },
    // The choice outlives the world it was made in.
    get plain() { return plain; },
    set plain(value: boolean) {
      plain = value;
      stage?.shade(value);
    },
    apply(frame: Frame) { stage?.apply(frame); },
    target(point: Point | undefined, shape: BrushShape, radius: number) { stage?.target(point, shape, radius); },
    draw(entered: boolean, yaw: number, pitch: number, dt: number, time: number) {
      if (!stage || covered || benching || backend === "starting") return;
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
      stage.draw(renderer, () => {
        renderStats.calls = renderer.info.render.drawCalls;
        renderStats.triangles = renderer.info.render.triangles;
      });
      cpuMs += (performance.now() - began - cpuMs) * 0.1;
      // The GPU answers a few frames late, and not at all where it cannot time itself.
      if (++drawn % 8 === 0) {
        void renderer.resolveTimestampsAsync("render").then((taken) => {
          if (typeof taken === "number" && taken > 0) gpuMs = gpuMs === null ? taken : gpuMs + (taken - gpuMs) * 0.2;
        });
      }
    },
    veil(shown: boolean) { stage?.veil(shown); },
    findWeather: (sought: WeatherSought) => stage?.findWeather(sought),
    levelCounts: () => stage?.counts() ?? [],
    air: () => stage?.air ?? null,
    cloudCost: () => null,
    // Time of a frame, as frames drawn back to back in batches of `frames` with the clouds shown and hidden in turn
    // until the GPU is done: the display's vsync cannot hold them back, so the difference is what the clouds cost.
    // Median milliseconds a frame over the batches; null where the device cannot be waited on. Showing the clouds
    // again draws both cloud shadow maps at once, so each batch begins with one frame left out of its time. The
    // light does not ease while the bench draws; the clouds' picture is built up again afterwards.
    async bench(frames: number, batches = 6) {
      const device = (renderer.backend as { device?: { queue: { onSubmittedWorkDone(): Promise<void> } } }).device;
      const benched = stage;
      if (!benched || !device || benching) return null;
      const timed = { cloudy: [] as number[], clear: [] as number[] };
      const shown = benched.cloudsShown;
      benching = true;
      benched.hold();
      try {
        for (let batch = 0; batch < batches * 2; batch++) {
          const cloudy = batch % 2 === 0;
          benched.veil(cloudy);
          benched.draw(renderer, () => {});
          await device.queue.onSubmittedWorkDone();
          const began = performance.now();
          for (let frame = 0; frame < frames; frame++) benched.draw(renderer, () => {});
          await device.queue.onSubmittedWorkDone();
          timed[cloudy ? "cloudy" : "clear"].push((performance.now() - began) / frames);
        }
      } finally {
        benched.veil(shown);
        benching = false;
      }
      const median = (values: number[]) => Number([...values].sort((a, b) => a - b)[values.length >> 1].toFixed(3));
      const cloudy = median(timed.cloudy);
      const clear = median(timed.clear);
      return { frames, batches, cloudyMs: cloudy, clearMs: clear, cloudMs: Number((cloudy - clear).toFixed(3)), ...timed };
    },
    timings: () => ({ cpuMs, gpuMs }),
    renderStats,
  };
}
