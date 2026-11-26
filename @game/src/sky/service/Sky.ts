import { DirectionalLight, FogExp2, HemisphereLight, type Scene, Vector2, Vector3, Vector4 } from "three";
import type { Point } from "../../../contract/GameBridge";
import { Adaptation, AirEnd, ExposedLight, Exposure, ExposureRange, HazeDepth, ReliefSun } from "../constant/air";
import { ShadowMap, ShadowReach } from "../constant/shadow";
import { dome } from "../fn/dome";
import { createSkyLight, setSkyLight } from "../fn/skyLight";
import type { Air } from "../type/Air";

const ShadowStep = 2;
const PlainAir = 0.35;
const luminance = (color: { r: number; g: number; b: number }) => 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;

// Sun, sky light, haze and the sky dome, all following the simulated sun. Everything here lives in render
// space, next to the camera.
export function createSky(scene: Scene, viewDistance: number, seaLevel: number) {
  const light = createSkyLight();
  const haze = HazeDepth / viewDistance;
  const air: Air = {
    airSea: { value: seaLevel },
    airShare: { value: 1 },
    airEnd: { value: new Vector2(viewDistance * AirEnd, viewDistance) },
    airVeil: { value: new Vector4() },
  };
  // What three.js's own materials fade by: the things drawn near the eye.
  const fog = new FogExp2(0, haze);
  let exposure = Exposure;
  const sky = dome(light);
  const ambient = new HemisphereLight();
  const sun = new DirectionalLight();
  const center = new Vector3(Infinity, 0, 0);
  const next = new Vector3();
  const towards = new Vector3(0, 1, 0);
  const shadow = sun.shadow.camera;
  let plain = false;
  // Share of the sky under cloud and that cloud's optical depth, eased.
  let veil = 0;
  let veilDepth = 0;

  sun.castShadow = true;
  sun.shadow.mapSize.set(ShadowMap, ShadowMap);
  sun.shadow.bias = -0.00025;
  // Slope acne grows with the texel, so the offset along the normal is a fixed share of one.
  sun.shadow.normalBias = 0.93 * 2 * ShadowReach / ShadowMap;
  // The frustum is a box of half extent ShadowReach around the shadow centre, seen from the sun.
  shadow.left = shadow.bottom = -ShadowReach;
  shadow.right = shadow.top = ShadowReach;
  shadow.far = ShadowReach * 3;
  // The haze is the horizon's colour by sharing its object, not a copy of it.
  fog.color = light.skyHorizon.value;
  scene.fog = fog;
  scene.add(sky, ambient, sun, sun.target);
  setSkyLight(light, ReliefSun.toArray());

  // Plain shading is a fixed white light for judging relief at any hour; daylight is the hour's own.
  function relight() {
    air.airShare.value = plain ? PlainAir : 1;
    air.airVeil.value.set(0, 0, 0, 0);
    exposure = Exposure;
    if (plain) {
      ambient.color.set("#ffffff");
      ambient.groundColor.set("#6f6f6f");
      ambient.intensity = 0.62;
      sun.color.set("#ffffff");
      sun.intensity = 3.3;
      towards.copy(ReliefSun);
    } else {
      // Under cloud the ground loses most of the blue sky's light and gains the sun's, spread over the whole sky
      // by the cloud: of sunlight falling on a layer of optical depth d about 2 / (2 + 0.15 d) comes out below.
      const through = Math.max(light.skySun.value.y, 0) * 2 / (2 + 0.15 * veilDepth) * veil;
      ambient.color.copy(light.skyAmbient.value).multiplyScalar(1 - 0.7 * veil);
      ambient.color.r += light.sunLight.value.r * through;
      ambient.color.g += light.sunLight.value.g * through;
      ambient.color.b += light.sunLight.value.b * through;
      ambient.groundColor.copy(light.groundAmbient.value).multiplyScalar(1 - 0.5 * veil);
      ambient.intensity = 1;
      sun.color.copy(light.sunLight.value);
      sun.intensity = 1;
      // Below the horizon the sun lights nothing; its shadow frustum is parked overhead.
      towards.copy(light.skySun.value.y > 0.02 ? light.skySun.value : next.set(0, 1, 0));
      // Under cloud the air between the eye and a far slope is lit by that cloud, not by blue sky.
      air.airVeil.value.set(ambient.color.r, ambient.color.g, ambient.color.b, veil).multiplyScalar(1 / Math.PI).setW(veil);
      // The eye and the camera follow the light: what falls on level ground decides how bright the picture is.
      const fallen = luminance(ambient.color) + luminance(sun.color) * Math.max(light.skySun.value.y, 0) * (1 - veil);
      exposure = Math.min(Math.max(Exposure * (ExposedLight / Math.max(fallen, 1e-3)) ** Adaptation, ExposureRange[0]), ExposureRange[1]);
    }
  }

  relight();

  return {
    light,
    air,
    haze,
    // What the tone mapping should multiply the light by.
    get exposure() { return exposure; },
    // The hour's light for a direction towards the sun. The sun creeps, so the light and the shadow map follow in
    // steps too small to see.
    turn(direction: Point) {
      if (light.skySun.value.distanceToSquared(next.set(...direction)) < 2.5e-7) return;
      setSkyLight(light, direction);
      relight();
      center.x = Infinity;
    },
    shade(value: boolean) {
      plain = value;
      relight();
      center.x = Infinity;
    },
    // The cloud overhead, eased over a second or two so a weather cell's border passing over does not flick the
    // light.
    cover(share: number, depth: number, dt: number) {
      if (!Number.isFinite(share + depth) || Math.abs(share - veil) + Math.abs(depth - veilDepth) < 0.002) return;
      const ease = 1 - Math.exp(-2 * dt);
      veil += (share - veil) * ease;
      veilDepth += (depth - veilDepth) * ease;
      if (!plain) relight();
    },
    // Centres the dome and the shadow frustum on the eye. Returns whether the shadow map is stale.
    follow(eye: Vector3) {
      sky.position.copy(eye);
      next.copy(eye).divideScalar(ShadowStep).round().multiplyScalar(ShadowStep);
      if (center.equals(next)) return false;
      center.copy(next);
      sun.target.position.copy(center);
      sun.position.copy(center).addScaledVector(towards, ShadowReach * 1.5);
      return true;
    },
    // Call when the render origin moves: render-space positions of everything lit have changed. `sea` is the
    // sea's height in render space.
    rebase(sea: number) {
      center.x = Infinity;
      air.airSea.value = sea;
    },
    dispose() {
      scene.fog = null;
      scene.remove(sky, ambient, sun, sun.target);
      sky.geometry.dispose();
      sky.material.dispose();
      sun.dispose();
      ambient.dispose();
    },
  };
}

export type Sky = ReturnType<typeof createSky>;
