import { Color, Group, type PerspectiveCamera, type Scene, Vector3, type WebGLRenderer } from "three";
import type { BrushShape, Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import { createBrushIndicator } from "../../brush/service/BrushIndicator";
import { findWeather, type WeatherSought } from "../../clouds/fn/findWeather";
import { createCloudPass } from "../../clouds/service/CloudPass";
import { createCloudShadow } from "../../clouds/service/CloudShadow";
import { createClouds } from "../../clouds/service/Clouds";
import { createGpuTimer } from "../../diagnostics/service/GpuTimer";
import { createParticleScene } from "../../particles/service/ParticleScene";
import { createSky } from "../../sky/service/Sky";
import { createFarTerrain } from "../../terrain/service/FarTerrain";
import { createTerrainShadow } from "../../terrain/service/TerrainShadow";
import { createNaturalWater } from "../../water/service/NaturalWater";
import { createWaterScene } from "../../water/service/WaterScene";
import { createPrecipitation } from "../../weather/service/Precipitation";
import { groundPalette } from "../fn/groundPalette";
import { anchorTone, renderOriginStep, surfaceTone } from "../fn/surfaceTone";
import { voxelSurface } from "../fn/voxelSurface";
import type { HandOff } from "../type/HandOff";
import { createVoxelOcclusion } from "./VoxelOcclusion";
import { createVoxelTerrain } from "./VoxelTerrain";

// A jump of the eye beyond this is a teleport or a respawn: cut to it instead of gliding.
const CutDistance = 250;

// Everything one world puts on screen. World positions are doubles and can be millions of metres from the world
// origin, so `content` is offset by a render origin near the camera: the matrices three.js hands to the GPU, the
// shadow frustum and every shader's "world" position are then small render-space numbers.
export function createWorldStage(renderer: WebGLRenderer, scene: Scene, camera: PerspectiveCamera, world: WorldInfo) {
  const step = renderOriginStep(world.voxelSize);
  const tone = surfaceTone(world.voxelSize);
  const palette = world.materials.map(({ color }) => new Color(color));
  const handOff: HandOff = { boxMin: { value: new Vector3(1, 1, 1) }, boxMax: { value: new Vector3(-1, -1, -1) } };
  const content = new Group();
  content.matrixAutoUpdate = false;
  scene.add(content);

  camera.far = world.far.viewDistance * 2;
  camera.updateProjectionMatrix();
  const sky = createSky(scene, world.far.viewDistance, world.seaLevel);
  const clouds = createClouds(renderer, camera.position);
  const cloudTimer = createGpuTimer(renderer);
  const cloudPass = createCloudPass(clouds.uniforms, sky.light, cloudTimer, camera.far, world.far.viewDistance * 1.6, sky.haze);
  const cloudShadow = createCloudShadow(clouds.uniforms, sky.light.skySun);
  renderer.setEffects([cloudPass]);

  const terrainShadow = createTerrainShadow(scene, sky.light.skySun);
  const shade = { ...cloudShadow.uniforms, ...terrainShadow.uniforms };
  const occlusion = createVoxelOcclusion(renderer, world);
  const chunkSurface = voxelSurface(tone, shade, sky.light, sky.air, { ...handOff, ...occlusion.uniforms });
  const pieceSurface = voxelSurface(tone, shade, sky.light, sky.air);
  const voxels = createVoxelTerrain(content, world, palette, chunkSurface, pieceSurface, occlusion);
  const paletteStrip = groundPalette(world);
  const far = createFarTerrain(content, world.far, {
    tone, handOff, shade, light: sky.light, air: sky.air, groundPalette: { value: paletteStrip },
  });
  const water = createWaterScene(content, world, palette);
  const natural = createNaturalWater(content, sky.light, sky.air, cloudShadow.uniforms, tone.plain);
  const particles = createParticleScene(content);
  const precipitation = createPrecipitation(scene, camera, sky.light);
  const brush = createBrushIndicator(content, world);

  const origin = new Vector3();
  const eyeTarget = new Vector3();
  const eye = new Vector3();
  let voxelBox: Frame["voxelBox"] = null;
  let ground: number | null = null;
  let placed = false;
  let plain = false;
  let cloudy = true;

  function rebase(around: Vector3) {
    if (Math.max(Math.abs(around.x - origin.x), Math.abs(around.y - origin.y), Math.abs(around.z - origin.z)) <= step) return;
    origin.copy(around).divideScalar(step).round().multiplyScalar(step);
    content.position.copy(origin).negate();
    content.updateMatrix();
    anchorTone(tone, origin);
    sky.rebase(world.seaLevel - origin.y);
  }

  return {
    // Plain shading shows the ground alone: neutral light, no clouds.
    shade(value: boolean) {
      plain = value;
      sky.shade(plain);
      tone.plain.value = Number(plain);
    },
    // False hides the clouds, their shadows and what falls from them; the weather itself goes on.
    veil(shown: boolean) { cloudy = shown; },
    findWeather: (sought: WeatherSought) => clouds.field.latest ? findWeather(clouds.field.latest, sought) : undefined,
    patchCounts: far.counts,
    openWaterCells: () => natural.openCells,
    // Milliseconds of GPU time the cloud pass takes, or null where the browser cannot say.
    cloudCost: () => cloudPass.enabled ? cloudTimer.milliseconds : 0,
    apply(frame: Frame) {
      voxels.apply(frame);
      if (far.apply(frame)) terrainShadow.touch();
      water.apply(frame);
      natural.apply(frame);
      particles.apply(frame.particles, frame.particleOrigin);
      eyeTarget.set(...frame.eye);
      voxelBox = frame.voxelBox;
      ground = frame.ground;
      sky.turn(frame.sun);
      clouds.apply(frame, performance.now());
      precipitation.apply(frame);
    },
    target(point: Point | undefined, shape: BrushShape, radius: number) {
      brush.show(point, shape, radius);
    },
    // Poses the camera and everything that follows it. Before the player has entered, the view pans slowly from
    // where they stand. Returns whether the shadow map must be redrawn.
    pose(entered: boolean, yaw: number, pitch: number, dt: number, time: number) {
      if (!placed || eye.distanceTo(eyeTarget) > CutDistance) {
        eye.copy(eyeTarget);
        cloudPass.forget();
      } else eye.lerp(eyeTarget, 1 - Math.exp(-32 * dt));
      placed = true;
      rebase(eye);
      camera.position.copy(eye).sub(origin);
      if (entered) camera.rotation.set(pitch, yaw, 0);
      else camera.rotation.set(-0.05, 0.6 + Math.sin(time * 0.05) * 0.4, 0);
      if (voxelBox) {
        handOff.boxMin.value.set(...voxelBox.min).sub(origin);
        handOff.boxMax.value.set(...voxelBox.max).sub(origin);
      } else {
        handOff.boxMin.value.setScalar(1);
        handOff.boxMax.value.setScalar(-1);
      }
      // The eye trails its target by a thirty-second of a second, which gives its speed.
      occlusion.update(origin, eye, eye.distanceTo(eyeTarget) * 32, dt);
      const now = performance.now();
      cloudPass.enabled = clouds.update(origin, eye, now) && !plain && cloudy;
      cloudPass.aim(camera, eye);
      cloudShadow.update(renderer, origin, eye, ground ?? eye.y, cloudPass.enabled, now);
      terrainShadow.update(renderer, origin, eye, !plain && sky.light.skySun.value.y > 0.02, now);
      precipitation.update(eye, now / 1000, !plain && cloudy);
      // With the clouds hidden the light is the clear sky's.
      const [share, depth] = cloudy ? clouds.field.overcast(eye.x, eye.z) : [0, 0];
      sky.cover(share, depth, dt);
      renderer.toneMappingExposure += (sky.exposure - renderer.toneMappingExposure) * Math.min(1, dt * 2);
      const moved = sky.follow(camera.position);
      return voxels.takeStale() || moved;
    },
    dispose() {
      renderer.setEffects(null);
      for (const part of [
        voxels, far, water, natural, particles, precipitation, brush, sky, clouds, cloudPass, cloudShadow, terrainShadow,
        chunkSurface, pieceSurface, paletteStrip, occlusion,
      ]) part.dispose();
      scene.remove(content);
    },
  };
}
