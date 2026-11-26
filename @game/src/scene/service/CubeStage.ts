import { Color, DirectionalLight, Group, HemisphereLight, type PerspectiveCamera, type Scene, Vector3, type WebGPURenderer } from "three/webgpu";
import { uniform } from "three/tsl";
import type { BrushShape, CloudField, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import { createBrushIndicator } from "../../brush/service/BrushIndicator";
import { findWeather, type WeatherSought } from "../../clouds/fn/findWeather";
import { createCloudPass } from "../../clouds/service/CloudPass";
import { createClouds } from "../../clouds/service/Clouds";
import { createCloudShadow } from "../../clouds/service/CloudShadow";
import { cubeSurface } from "../../cubes/fn/cubeSurface";
import { createLevelTable } from "../../cubes/fn/levelTable";
import { createCubeTerrain } from "../../cubes/service/CubeTerrain";
import { createLevelHeights } from "../../cubes/service/LevelHeights";
import { createParticleScene } from "../../particles/service/ParticleScene";
import { cascadeOf } from "../../shadow/constant/cascades";
import { createSunShadow } from "../../shadow/service/SunShadow";
import { skyBackground } from "../../sky/fn/radiance";
import { createDaylight } from "../../sky/service/Daylight";
import { createSnowField } from "../../snow/service/SnowField";
import { sheetSurface } from "../../water/fn/sheetSurface";
import { voxelWaterSurface } from "../../water/fn/voxelWaterSurface";
import { createVoxelWater } from "../../water/service/VoxelWater";
import { createPrecipitation } from "../../weather/service/Precipitation";
import { GroundRepeat } from "../constant/grain";
import { groundPalette } from "../fn/groundPalette";
import { renderOriginStep } from "../fn/renderOriginStep";

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
  const viewDistance = world.cubes?.viewDistance ?? world.far.viewDistance;
  camera.far = viewDistance * 2;
  camera.updateProjectionMatrix();

  // A recording made before the clean air was part of the world cannot be drawn in it.
  if (!world.air) throw new Error("This world has no clean air (WorldInfo.air): its addon or recording predates it");
  const daylight = createDaylight(viewDistance, world.air);
  daylight.rebase(world.seaLevel);
  const sky = daylight.nodes;
  scene.backgroundNode = skyBackground(sky);
  // three.js's own materials, which the particles and the brush use, are lit by these two; their sun takes the cloud
  // over the eye.
  const ambient = new HemisphereLight();
  const sun = new DirectionalLight();
  scene.add(ambient, sun);

  const anchor = uniform(new Vector3());
  const levels = createLevelTable();
  const palette = groundPalette(world);
  const clouds = createClouds();
  const cloudShadow = createCloudShadow(clouds.nodes, sky.skySun);
  const shadow = createSunShadow(world, levels, cloudShadow.shade);
  const cloudPass = createCloudPass(clouds.nodes, sky, viewDistance);
  const snow = createSnowField(world);
  const heights = createLevelHeights(world);
  const surface = cubeSurface({ sky, levels, palette, anchor, shadow: shadow.nodes, snow: snow.nodes, land: heights.nodes });
  const sheet = sheetSurface(sky, levels, cloudShadow.shade);
  const terrain = createCubeTerrain(content, world, levels, surface, sheet, shadow.casting, heights);
  const liquid = world.materials.find(({ matter }) => matter === "liquid")?.color ?? 0x2f939b;
  const wet = voxelWaterSurface(sky, cloudShadow.shade, new Color(liquid));
  const water = createVoxelWater(content, world, wet);
  const particles = createParticleScene(content);
  const brush = createBrushIndicator(content, world);
  const precipitation = createPrecipitation(scene, camera, sky);

  const origin = new Vector3();
  const eyeTarget = new Vector3();
  const eye = new Vector3();
  let field: CloudField | undefined;
  let ground: number | null = null;
  let plain = false;
  let cloudy = true;
  let dt = 0;
  let squares: ReturnType<typeof terrain.place> = [];
  let placed = false;

  function rebase(around: Vector3) {
    if (Math.max(Math.abs(around.x - origin.x), Math.abs(around.y - origin.y), Math.abs(around.z - origin.z)) <= step) return;
    origin.copy(around).divideScalar(step).round().multiplyScalar(step);
    content.position.copy(origin).negate();
    content.updateMatrix();
    const within = (value: number) => value - Math.floor(value / GroundRepeat) * GroundRepeat;
    anchor.value.set(within(origin.x), within(origin.y), within(origin.z));
    daylight.rebase(world.seaLevel - origin.y);
    snow.rebase(origin);
  }

  return {
    // Plain shading shows the ground alone: neutral light and thin air.
    shade(value: boolean) {
      plain = value;
      daylight.shade(value);
    },
    // False hides the clouds, their shadows and what falls from them; the weather itself goes on.
    veil(shown: boolean) { cloudy = shown; },
    findWeather: (sought: WeatherSought) => field ? findWeather(field, sought) : undefined,
    counts: terrain.counts,
    get exposure() { return daylight.exposure; },
    get air() { return daylight.air; },
    apply(frame: Frame) {
      terrain.apply(frame);
      water.apply(frame);
      particles.apply(frame.particles, frame.particleOrigin);
      eyeTarget.set(...frame.eye);
      daylight.turn(frame.sun, frame.ground === null ? null : frame.ground - world.seaLevel);
      ground = frame.ground;
      clouds.apply(frame, performance.now());
      precipitation.apply(frame);
      if (frame.clouds && frame.clouds !== field) {
        field = frame.clouds;
        snow.apply(field);
        snow.rebase(origin);
      }
    },
    target(point: Point | undefined, shape: BrushShape, radius: number) {
      brush.show(point, shape, radius);
    },
    // Poses the camera and everything that follows it. Before the player has entered, the view pans slowly from
    // where they stand.
    pose(entered: boolean, yaw: number, pitch: number, elapsed: number, time: number) {
      dt = elapsed;
      if (!placed || eye.distanceTo(eyeTarget) > CutDistance) {
        eye.copy(eyeTarget);
        cloudPass.forget();
      } else eye.lerp(eyeTarget, 1 - Math.exp(-32 * dt));
      placed = true;
      rebase(eye);
      camera.position.copy(eye).sub(origin);
      if (entered) camera.rotation.set(pitch, yaw, 0);
      else camera.rotation.set(-0.05, 0.6 + Math.sin(time * 0.05) * 0.4, 0);
      squares = terrain.place(origin);
      ambient.color.copy(sky.skyLight.value);
      ambient.groundColor.copy(sky.groundLight.value);
      sun.color.copy(sky.sunLight.value).multiplyScalar(cloudShadow.shade.beyond.value);
      sun.position.copy(sky.sunDirection.value);
    },
    // Draws the view: the shadow maps that are due, the scene, then the clouds over it onto the screen. `counted`
    // runs straight after the scene is drawn, while the renderer's counts are the scene's. Call after pose.
    draw(renderer: WebGPURenderer, counted: () => void) {
      const now = performance.now();
      const shown = clouds.update(renderer, origin, eye, now) && !plain && cloudy;
      // With the clouds hidden the light is the clear sky's.
      const overhead = shown ? clouds.field.overcast(eye.x, eye.z) : [0, 0] as const;
      cloudShadow.update(renderer, origin, eye, ground ?? eye.y, shown, now, overhead);
      daylight.cover(overhead[0], overhead[1], dt);
      daylight.haze(clouds.field.air(eye.x, eye.z), dt);
      precipitation.update(eye, now / 1000, !plain && cloudy);
      shadow.cast(renderer, {
        origin, eye, ground, sun: sky.sunDirection.value, squares,
        stamp: (cascade) => terrain.stamp((level) => cascadeOf(level) === cascade),
      });
      cloudPass.scene(renderer, scene, camera);
      counted();
      cloudPass.finish(renderer, camera, eye, shown);
    },
    dispose() {
      const parts = [terrain, water, particles, brush, surface, sheet, wet, palette, ambient, sun, shadow, snow, clouds, cloudShadow, cloudPass];
      for (const part of [...parts, precipitation]) {
        part.dispose();
      }
      heights.dispose();
      scene.backgroundNode = null;
      scene.remove(content, ambient, sun);
    },
  };
}
