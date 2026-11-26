import { ACESFilmicToneMapping, HalfFloatType, PCFShadowMap, PerspectiveCamera, Scene, WebGLRenderer } from "three";
import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import type { WeatherSought } from "../../clouds/fn/findWeather";
import { createWorldStage } from "./WorldStage";

// The renderer and camera outlive worlds; `load` swaps the stage. The depth buffer is logarithmic because one
// view holds voxels 6 cm from the eye and mountains hundreds of kilometres away. The scene is drawn into a
// half-float buffer whose colour and depth the cloud pass reads; tone mapping happens once, after it.
export function createWorldScene(canvas: HTMLCanvasElement) {
  const renderer = new WebGLRenderer({
    canvas, antialias: true, powerPreference: "high-performance", logarithmicDepthBuffer: true,
    outputBufferType: HalfFloatType,
  });
  // One frame is several render calls; its statistics are reset by hand so they add up.
  renderer.info.autoReset = false;
  renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5));
  renderer.toneMapping = ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = PCFShadowMap;
  renderer.shadowMap.autoUpdate = false;
  renderer.setClearColor("#111e1a");

  const scene = new Scene();
  scene.matrixAutoUpdate = false;
  const camera = new PerspectiveCamera(62, 1, 0.055, 1000);
  camera.rotation.order = "YXZ";
  let stage: ReturnType<typeof createWorldStage> | undefined;
  let plain = false;
  let covered = false;
  let width = 0;
  let height = 0;

  return {
    // What draws; the F3 panel and every shot record say it.
    backend: "WebGL 2" as string,
    load(world: WorldInfo) {
      stage?.dispose();
      stage = createWorldStage(renderer, scene, camera, world);
      stage.shade(plain);
    },
    // Leaves the canvas blank: the launcher is what shows until the next world loads.
    unload() {
      stage?.dispose();
      stage = undefined;
      renderer.clear();
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
      if (!stage || covered) return;
      if (canvas.clientWidth !== width || canvas.clientHeight !== height) {
        width = canvas.clientWidth;
        height = canvas.clientHeight;
        renderer.setSize(width, height, false);
        camera.aspect = width / Math.max(1, height);
        camera.updateProjectionMatrix();
      }
      renderer.shadowMap.needsUpdate = stage.pose(entered, yaw, pitch, dt, time);
      renderer.info.reset();
      renderer.render(scene, camera);
    },
    veil(shown: boolean) { stage?.veil(shown); },
    findWeather: (sought: WeatherSought) => stage?.findWeather(sought),
    patchCounts: () => stage?.patchCounts() ?? [],
    openWaterCells: () => stage?.openWaterCells() ?? 0,
    cloudCost: () => stage?.cloudCost() ?? 0,
    // Milliseconds a frame's draw takes on the CPU and on the GPU; null where that is not measured.
    timings: (): { cpuMs: number | null; gpuMs: number | null } => ({ cpuMs: null, gpuMs: null }),
    renderStats: renderer.info.render,
  };
}
