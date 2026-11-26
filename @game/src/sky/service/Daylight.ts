import { Color, Vector2, Vector3, Vector4 } from "three/webgpu";
import { uniform } from "three/tsl";
import type { Point } from "../../../contract/GameBridge";
import { Adaptation, AirEnd, ExposedLight, Exposure, ExposureRange, GroundBounce, GroundTint, ReliefSun, StillAir } from "../constant/air";
import { createSkyLight, setSkyLight } from "../fn/skyLight";
import type { AirOptics } from "../type/AirOptics";
import type { HazeAir } from "../type/HazeAir";
import type { SeenAir } from "../type/SeenAir";
import type { SkyNodes } from "../type/SkyNodes";

const PlainAir = 0.35;
// Metres the ground under the eye may rise or fall before its light is worked out again.
const AltitudeStep = 25;
// Heights of the sun (sines) over which its beam is faded out: below the lower one the shadows and faces are lit as
// if it stood overhead, so a beam left on there would light level ground in full.
const Setting = [0.02, 0.05] as const;
const luminance = (color: Color) => 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;

// Sun, sky light, air and exposure, all following the simulated sun, as the uniforms node materials light
// themselves from. Everything here lives in render space. `optics` is the simulation's clean air.
export function createDaylight(viewDistance: number, optics: AirOptics) {
  const light = createSkyLight();
  const nodes: SkyNodes = {
    skySun: uniform(light.skySun.value),
    skyZenith: uniform(light.skyZenith.value),
    skyHorizon: uniform(light.skyHorizon.value),
    skyGlow: uniform(light.skyGlow.value),
    sunDisc: uniform(light.sunLight.value),
    clearSky: uniform(light.domeLight.value),
    sunDirection: uniform(new Vector3(0, 1, 0)),
    sunLight: uniform(new Color()),
    skyLight: uniform(new Color()),
    groundLight: uniform(new Color()),
    airSea: uniform(0),
    airHaze: uniform(new Vector3()),
    airHazeSpectrum: uniform(new Vector3(1, 1, 1)),
    airClean: uniform(new Vector4(optics.cleanAir[0], optics.cleanAir[1], optics.cleanAir[2], optics.cleanAirHeight)),
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
  // Height above the sea of the ground under the eye, where the light is worked out.
  let altitude = 0;
  // The weather's haze over the eye, eased.
  const air: HazeAir = { ...StillAir, spectrum: [...StillAir.spectrum], albedo: [...StillAir.albedo] };
  let weathered = false;
  const next = new Vector3();
  // Plain shading keeps one air whatever the weather.
  const drawn = () => plain ? StillAir : air;

  function breathe() {
    const shown = drawn();
    nodes.airHaze.value.set(shown.haze / shown.thickness, shown.top, shown.veil);
    nodes.airHazeSpectrum.value.set(...shown.spectrum);
  }

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
    // The clouds' own shadows take the direct sun where they stand; it is not dimmed here as well.
    const rising = Math.max(0, Math.min(1, (light.skySun.value.y - Setting[0]) / (Setting[1] - Setting[0])));
    nodes.sunLight.value.copy(light.sunLight.value).multiplyScalar(rising * rising * (3 - 2 * rising));
    // Below the horizon the sun lights nothing.
    if (light.skySun.value.y > Setting[0]) nodes.sunDirection.value.copy(light.skySun.value);
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

  setSkyLight(light, ReliefSun.toArray(), altitude, drawn(), optics);
  breathe();
  relight();

  return {
    nodes,
    // What the tone mapping should multiply the light by.
    get exposure() { return exposure; },
    // The hour's light for a direction towards the sun, over ground `ground` metres above the sea (the last
    // height known when null). The sun creeps, so the light follows in steps too small to see.
    turn(direction: Point, ground: number | null) {
      const moved = ground !== null && Math.abs(ground - altitude) > AltitudeStep;
      if (!moved && light.skySun.value.distanceToSquared(next.set(...direction)) < 2.5e-7) return;
      if (ground !== null) altitude = ground;
      setSkyLight(light, direction, altitude, drawn(), optics);
      relight();
    },
    // The weather's haze over the eye, eased over a second or two like the cloud.
    haze(seen: HazeAir | undefined, dt: number) {
      if (!seen || ![seen.haze, seen.top, seen.veil, ...seen.spectrum, ...seen.albedo].every(Number.isFinite) || !(seen.thickness > 0)) return;
      weathered = true;
      let change = Math.abs(seen.haze - air.haze) + Math.abs(seen.veil - air.veil) + Math.abs(seen.top - air.top) / 1000;
      for (let colour = 0; colour < 3; colour++) {
        change += Math.abs(seen.spectrum[colour] - air.spectrum[colour]) + Math.abs(seen.albedo[colour] - air.albedo[colour]);
      }
      if (change < 0.0005 && seen.thickness === air.thickness) return;
      const ease = 1 - Math.exp(-2 * dt);
      air.haze += (seen.haze - air.haze) * ease;
      air.veil += (seen.veil - air.veil) * ease;
      air.top += (seen.top - air.top) * ease;
      for (let colour = 0; colour < 3; colour++) {
        air.spectrum[colour] += (seen.spectrum[colour] - air.spectrum[colour]) * ease;
        air.albedo[colour] += (seen.albedo[colour] - air.albedo[colour]) * ease;
      }
      air.thickness = seen.thickness;
      breathe();
      setSkyLight(light, light.skySun.value.toArray(), altitude, drawn(), optics);
      relight();
    },
    shade(value: boolean) {
      plain = value;
      setSkyLight(light, light.skySun.value.toArray(), altitude, drawn(), optics);
      breathe();
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
    // The weather's haze as the light and the air are drawn with it.
    // Null until the first weather snapshot has reached it.
    get air(): SeenAir | null { return weathered ? { haze: air, optics } : null; },
    // `sea` is the sea's height in render space; call when the render origin moves.
    rebase(sea: number) { nodes.airSea.value = sea; },
  };
}

export type Daylight = ReturnType<typeof createDaylight>;
