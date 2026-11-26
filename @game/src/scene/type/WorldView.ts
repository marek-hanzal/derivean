import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import type { WeatherSought } from "../../clouds/fn/findWeather";
import type { SeenAir } from "../../sky/type/SeenAir";

// What draws the world (CubeScene): the session feeds it frames and poses, the HUD and captures read it.
export interface WorldView {
  // What three.js got: WebGPU, its WebGL 2 fallback, or "starting" until the device is there.
  readonly backend: string;
  load(world: WorldInfo): void;
  unload(): void;
  // True while something opaque lies over the canvas, as the map does: the world is kept but not drawn.
  covered: boolean;
  // Plain relief shading.
  plain: boolean;
  apply(frame: Frame): void;
  target(point: Point | undefined, shape: BrushShape, radius: number): void;
  // `entered` is false until the player first takes control of this world.
  draw(entered: boolean, yaw: number, pitch: number, dt: number, time: number): void;
  // Shows or hides the clouds.
  veil(shown: boolean): void;
  findWeather(sought: WeatherSought): [number, number] | undefined;
  // Chunks held by each level of cubes.
  levelCounts(): number[];
  // The weather's haze the air is drawn with, null before the first weather snapshot.
  air(): SeenAir | null;
  // GPU milliseconds the clouds take, null while they are not drawn or not timed.
  cloudCost(): number | null;
  // Milliseconds the draw takes on the CPU and on the GPU; null where that is not measured.
  timings(): { cpuMs: number | null; gpuMs: number | null };
  readonly renderStats: { calls: number; triangles: number };
}
