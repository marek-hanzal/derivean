import { Color, DirectionalLight, Group, HemisphereLight, type PerspectiveCamera, type Scene, Vector3 } from "three/webgpu";
import { uniform } from "three/tsl";
import type { BrushShape, CloudField, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import { createBrushIndicator } from "../../brush/service/BrushIndicator";
import { findWeather, type WeatherSought } from "../../clouds/fn/findWeather";
import { cubeSurface } from "../../cubes/fn/cubeSurface";
import { createLevelTable } from "../../cubes/fn/levelTable";
import { createCubeTerrain } from "../../cubes/service/CubeTerrain";
import { createParticleScene } from "../../particles/service/ParticleScene";
import { skyBackground } from "../../sky/fn/radiance";
import { createDaylight } from "../../sky/service/Daylight";
import { sheetSurface } from "../../water/fn/sheetSurface";
import { voxelWaterSurface } from "../../water/fn/voxelWaterSurface";
import { createVoxelWater } from "../../water/service/VoxelWater";
import { GroundRepeat } from "../constant/grain";
import { groundPalette } from "../fn/groundPalette";
import { renderOriginStep } from "../fn/surfaceTone";

// A jump of the eye beyond this is a teleport or a respawn: cut to it instead of gliding.
const CutDistance = 250;

// Everything one world puts on screen. World positions are doubles and can be millions of metres from the world
// origin, so `content` is offset by a render origin near the camera: the matrices handed to the GPU and every
// shader's "world" position are then small render-space numbers. The origin moves in whole repeats of the cubes'
// grain.
export function createCubeStage(scene: Scene, camera: PerspectiveCamera, world: WorldInfo) {
  const step = renderOriginStep(world.voxelSize);
  const content = new Group();
  content.matrixAutoUpdate = false;
  scene.add(content);
  camera.far = world.far.viewDistance * 2;
  camera.updateProjectionMatrix();

  const daylight = createDaylight(world.far.viewDistance);
  const sky = daylight.nodes;
  scene.backgroundNode = skyBackground(sky);
  // three.js's own materials, which the particles and the brush still use, are lit by these two.
  const ambient = new HemisphereLight();
  const sun = new DirectionalLight();
  scene.add(ambient, sun);

  const anchor = uniform(new Vector3());
  const levels = createLevelTable();
  const palette = groundPalette(world);
  const surface = cubeSurface({ sky, levels, palette, anchor });
  const sheet = sheetSurface(sky, levels);
  const terrain = createCubeTerrain(content, world, levels, surface, sheet);
  const wet = voxelWaterSurface(sky, new Color(world.materials.find(({ matter }) => matter === "liquid")?.color ?? 0x2f939b));
  const water = createVoxelWater(content, world, wet);
  const particles = createParticleScene(content);
  const brush = createBrushIndicator(content, world);

  const origin = new Vector3();
  const eyeTarget = new Vector3();
  const eye = new Vector3();
  let clouds: CloudField | undefined;
  let placed = false;

  function rebase(around: Vector3) {
    if (Math.max(Math.abs(around.x - origin.x), Math.abs(around.y - origin.y), Math.abs(around.z - origin.z)) <= step) return;
    origin.copy(around).divideScalar(step).round().multiplyScalar(step);
    content.position.copy(origin).negate();
    content.updateMatrix();
    const within = (value: number) => value - Math.floor(value / GroundRepeat) * GroundRepeat;
    anchor.value.set(within(origin.x), within(origin.y), within(origin.z));
    daylight.rebase(world.seaLevel - origin.y);
  }

  return {
    // Plain shading shows the ground alone: neutral light and thin air.
    shade(value: boolean) { daylight.shade(value); },
    findWeather: (sought: WeatherSought) => clouds ? findWeather(clouds, sought) : undefined,
    counts: terrain.counts,
    get exposure() { return daylight.exposure; },
    apply(frame: Frame) {
      terrain.apply(frame);
      water.apply(frame);
      particles.apply(frame.particles, frame.particleOrigin);
      eyeTarget.set(...frame.eye);
      daylight.turn(frame.sun);
      clouds = frame.clouds ?? clouds;
    },
    target(point: Point | undefined, shape: BrushShape, radius: number) {
      brush.show(point, shape, radius);
    },
    // Poses the camera and everything that follows it. Before the player has entered, the view pans slowly from
    // where they stand.
    pose(entered: boolean, yaw: number, pitch: number, dt: number, time: number) {
      if (!placed || eye.distanceTo(eyeTarget) > CutDistance) eye.copy(eyeTarget);
      else eye.lerp(eyeTarget, 1 - Math.exp(-32 * dt));
      placed = true;
      rebase(eye);
      camera.position.copy(eye).sub(origin);
      if (entered) camera.rotation.set(pitch, yaw, 0);
      else camera.rotation.set(-0.05, 0.6 + Math.sin(time * 0.05) * 0.4, 0);
      terrain.place(origin);
      ambient.color.copy(sky.skyLight.value);
      ambient.groundColor.copy(sky.groundLight.value);
      sun.color.copy(sky.sunLight.value);
      sun.position.copy(sky.sunDirection.value);
    },
    dispose() {
      for (const part of [terrain, water, particles, brush, surface, sheet, wet, palette, ambient, sun]) part.dispose();
      scene.backgroundNode = null;
      scene.remove(content, ambient, sun);
    },
  };
}
