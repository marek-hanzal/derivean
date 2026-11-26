import { Color, Vector3 } from "three";
import type { Point } from "../../../contract/GameBridge";
import { AirDepth, DaySky, DuskSky, NightSky } from "../constant/air";
import type { SkyLight } from "../type/SkyLight";

const SunStrength = 2.7;
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
    groundAmbient: { value: new Color() },
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

// Sets the light for a sun direction. The sun's colour is its path through the air (Kasten and Young's air mass
// against AirDepth); the sky's is blended between day, dusk and night by the sun's height.
export function setSkyLight(light: SkyLight, sun: Point) {
  const height = sun[1];
  const days = smooth(-0.02, 0.3, height);
  const nights = 1 - smooth(-0.24, -0.04, height);
  const dusks = Math.max(0, 1 - days - nights);
  light.skySun.value.set(...sun).normalize();
  blend(light.skyZenith.value, DaySky.zenith, DuskSky.zenith, NightSky.zenith, days, dusks, nights);
  blend(light.skyHorizon.value, DaySky.horizon, DuskSky.horizon, NightSky.horizon, days, dusks, nights);
  blend(light.skyGlow.value, DaySky.glow, DuskSky.glow, NightSky.glow, days * 0.25, dusks, 0);

  const degrees = Math.max(height, 0) * 180 / Math.PI;
  const airMass = 1 / (Math.max(height, 0) + 0.50572 * (degrees + 6.07995) ** -1.6364);
  const risen = SunStrength * smooth(-0.035, 0.05, height);
  light.sunLight.value.setRGB(
    risen * Math.exp(-AirDepth[0] * airMass), risen * Math.exp(-AirDepth[1] * airMass), risen * Math.exp(-AirDepth[2] * airMass),
  );
  // The dome lights the ground with a blend of its zenith and horizon; the night keeps enough to walk by.
  const ambient = 1.9 * days + 1.1 * dusks + 0.5 * nights;
  light.skyAmbient.value.copy(light.skyZenith.value).lerp(light.skyHorizon.value, 0.55).multiplyScalar(ambient);
  light.groundAmbient.value.set("#8b6952").multiplyScalar(0.75 * days + 0.3 * dusks + 0.04 * nights);
}
