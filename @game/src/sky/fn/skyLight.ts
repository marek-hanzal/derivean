import { Color, Vector3 } from "three/webgpu";
import type { Point } from "../../../contract/GameBridge";
import { DaySky, DuskSky, NightSky } from "../constant/air";
import { clearSky } from "./clearSky";
import type { SkyLight } from "../type/SkyLight";

// The sun's light outside the air.
const SunStrength = 2.7;
// What the night sky lights the ground with, against its own colour: enough to walk by.
const NightLight = 0.5;
const scratch = new Color();
const smooth = (from: number, to: number, value: number) => {
  const t = Math.max(0, Math.min(1, (value - from) / (to - from)));
  return t * t * (3 - 2 * t);
};

export function createSkyLight(): SkyLight {
  return {
    skySun: { value: new Vector3(0, 1, 0) },
    skyZenith: { value: new Color() },
    skyHorizon: { value: new Color() },
    skyGlow: { value: new Color() },
    sunLight: { value: new Color() },
    skyAmbient: { value: new Color() },
  };
}

function blend(target: Color, day: string, dusk: string, night: string, days: number, dusks: number, nights: number) {
  target.setRGB(0, 0, 0);
  for (const [hex, share] of [[day, days], [dusk, dusks], [night, nights]] as const) {
    scratch.set(hex);
    target.r += scratch.r * share;
    target.g += scratch.g * share;
    target.b += scratch.b * share;
  }
}

// Sets the light for a sun direction over ground `altitude` metres above the sea. The sun's light and the clear
// sky's on level ground are what the air leaves of the sun's (clearSky); the sky's colours, which the background
// and the aerial perspective draw, are blended between day, dusk and night by the sun's height.
export function setSkyLight(light: SkyLight, sun: Point, altitude: number) {
  const height = sun[1];
  const days = smooth(-0.02, 0.3, height);
  const nights = 1 - smooth(-0.24, -0.04, height);
  const dusks = Math.max(0, 1 - days - nights);
  light.skySun.value.set(...sun).normalize();
  blend(light.skyZenith.value, DaySky.zenith, DuskSky.zenith, NightSky.zenith, days, dusks, nights);
  blend(light.skyHorizon.value, DaySky.horizon, DuskSky.horizon, NightSky.horizon, days, dusks, nights);
  blend(light.skyGlow.value, DaySky.glow, DuskSky.glow, NightSky.glow, days * 0.25, dusks, 0);

  const air = clearSky(light.skySun.value.y, altitude);
  light.sunLight.value.setRGB(...air.direct).multiplyScalar(SunStrength);
  // The night sky, a blend of its zenith and horizon, under what the sun's light leaves in the air.
  light.skyAmbient.value.set(NightSky.zenith).lerp(scratch.set(NightSky.horizon), 0.55).multiplyScalar(NightLight * nights);
  light.skyAmbient.value.r += air.diffuse[0] * SunStrength;
  light.skyAmbient.value.g += air.diffuse[1] * SunStrength;
  light.skyAmbient.value.b += air.diffuse[2] * SunStrength;
}
