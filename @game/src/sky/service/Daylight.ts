import { Color, Vector2, Vector3, Vector4 } from "three/webgpu";
import { uniform } from "three/tsl";
import type { Point } from "../../../contract/GameBridge";
import { Adaptation, AirEnd, ExposedLight, Exposure, ExposureRange, GroundBounce, GroundTint, ReliefSun } from "../constant/air";
import { createSkyLight, setSkyLight } from "../fn/skyLight";
import type { SkyNodes } from "../type/SkyNodes";

const PlainAir = 0.35;
const luminance = (color: Color) => 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;

// Sun, sky light, air and exposure, all following the simulated sun, as the uniforms node materials light
// themselves from. Everything here lives in render space.
export function createDaylight(viewDistance: number) {
  const light = createSkyLight();
  const nodes: SkyNodes = {
    skySun: uniform(light.skySun.value),
    skyZenith: uniform(light.skyZenith.value),
    skyHorizon: uniform(light.skyHorizon.value),
    skyGlow: uniform(light.skyGlow.value),
    sunDisc: uniform(light.sunLight.value),
    sunDirection: uniform(new Vector3(0, 1, 0)),
    sunLight: uniform(new Color()),
    skyLight: uniform(new Color()),
    groundLight: uniform(new Color()),
    airSea: uniform(0),
    airShare: uniform(1),
    airEnd: uniform(new Vector2(viewDistance * AirEnd, viewDistance)),
    airVeil: uniform(new Vector4()),
    plain: uniform(0),
  };
  let exposure = Exposure;
  let plain = false;
  // Share of the sky under cloud and that cloud's optical depth, eased.
  let veil = 0;
  let veilDepth = 0;
  const next = new Vector3();

  // Plain shading is a fixed white light for judging relief at any hour; daylight is the hour's own.
  function relight() {
    nodes.plain.value = Number(plain);
    nodes.airShare.value = plain ? PlainAir : 1;
    nodes.airVeil.value.set(0, 0, 0, 0);
    exposure = Exposure;
    if (plain) {
      nodes.skyLight.value.setScalar(0.62);
      nodes.groundLight.value.set("#6f6f6f").multiplyScalar(0.62);
      nodes.sunLight.value.setScalar(3.3);
      nodes.sunDirection.value.copy(ReliefSun);
      return;
    }
    const height = Math.max(light.skySun.value.y, 0);
    // Under cloud the ground loses most of the blue sky's light and gains the sun's, spread over the whole sky by
    // the cloud: of sunlight falling on a layer of optical depth d about 2 / (2 + 0.15 d) comes out below.
    const through = height * 2 / (2 + 0.15 * veilDepth) * veil;
    const sky = nodes.skyLight.value.copy(light.skyAmbient.value).multiplyScalar(1 - 0.7 * veil);
    sky.r += light.sunLight.value.r * through;
    sky.g += light.sunLight.value.g * through;
    sky.b += light.sunLight.value.b * through;
    nodes.sunLight.value.copy(light.sunLight.value).multiplyScalar(1 - veil);
    // Below the horizon the sun lights nothing.
    if (light.skySun.value.y > 0.02) nodes.sunDirection.value.copy(light.skySun.value);
    else nodes.sunDirection.value.set(0, 1, 0);
    // Under cloud the air between the eye and a far slope is lit by that cloud, not by blue sky.
    nodes.airVeil.value.set(sky.r / Math.PI, sky.g / Math.PI, sky.b / Math.PI, veil);
    // The eye and the camera follow the light: what falls on level ground decides how bright the picture is.
    const fallen = luminance(sky) + luminance(light.sunLight.value) * height * (1 - veil);
    // Half of the faces of a world of cubes are upright and see as much ground as sky: the ground's share of
    // what falls on it lights them from below.
    const ground = nodes.groundLight.value.set(GroundTint);
    ground.multiplyScalar(fallen * GroundBounce / luminance(ground));
    exposure = Math.min(Math.max(Exposure * (ExposedLight / Math.max(fallen, 1e-3)) ** Adaptation, ExposureRange[0]), ExposureRange[1]);
  }

  setSkyLight(light, ReliefSun.toArray());
  relight();

  return {
    nodes,
    // What the tone mapping should multiply the light by.
    get exposure() { return exposure; },
    // The hour's light for a direction towards the sun. The sun creeps, so the light follows in steps too small
    // to see.
    turn(direction: Point) {
      if (light.skySun.value.distanceToSquared(next.set(...direction)) < 2.5e-7) return;
      setSkyLight(light, direction);
      relight();
    },
    shade(value: boolean) {
      plain = value;
      relight();
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
    // `sea` is the sea's height in render space; call when the render origin moves.
    rebase(sea: number) { nodes.airSea.value = sea; },
  };
}

export type Daylight = ReturnType<typeof createDaylight>;
