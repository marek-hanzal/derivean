import { Color, Vector3 } from "three/webgpu";
import type { Point } from "../../../contract/GameBridge";
import { DaySky, DuskSky, NightSky, ReferenceDepth, ReferenceHaze, VeilHeight, ZenithPaling } from "../constant/air";
import { clearSky, hazeAbove } from "./clearSky";
import type { AirOptics } from "../type/AirOptics";
import type { HazeAir } from "../type/HazeAir";
import type { SkyLight } from "../type/SkyLight";

// The sun's light outside the air.
const SunStrength = 2.7;
// What the night sky lights the ground with, against its own colour: enough to walk by at the exposure's top.
const NightLight = 0.35;
// The dome's light against its colours, by day, dusk and night.
const DomeLight = { day: 1.9, dusk: 1.1, night: 0.5 };
const scratch = new Color();
const tint: [number, number, number] = [0, 0, 0];
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
    domeLight: { value: new Color() },
  };
}

function add(target: Color, hex: string, share: number) {
  scratch.set(hex);
  target.r += scratch.r * share;
  target.g += scratch.g * share;
  target.b += scratch.b * share;
}

function blend(target: Color, day: string, dusk: string, night: string, days: number, dusks: number, nights: number) {
  target.setRGB(0, 0, 0);
  add(target, day, days);
  add(target, dusk, dusks);
  add(target, night, nights);
}

// What the haze and the clean air's green take from light per metre along a level line at a height.
function levelLoss(air: HazeAir, altitude: number, optics: AirOptics) {
  const haze = (altitude < air.top ? air.haze / air.thickness : 0) + air.veil / VeilHeight * Math.exp(-Math.max(altitude, 0) / VeilHeight);
  return { haze, clean: optics.cleanAir[1] * Math.exp(-Math.max(altitude, 0) / optics.cleanAirHeight) };
}

// Share of what the air takes from light along a level line at a height that the haze takes.
export function hazeShare(air: HazeAir, altitude: number, optics: AirOptics) {
  const { haze, clean } = levelLoss(air, altitude, optics);
  return haze / (haze + clean);
}

// Metres along a level line at a height at which a dark target fades to 2 % contrast in the drawn air (Koschmieder).
export function levelView(air: HazeAir, altitude: number, optics: AirOptics) {
  const { haze, clean } = levelLoss(air, altitude, optics);
  return -Math.log(0.02) / (haze + clean);
}

// Sets the light for a sun direction over ground `altitude` metres above the sea under the weather's haze `air`.
// The sun's light and the clear sky's on level ground are what the air leaves of the sun's (clearSky); the sky's
// colours, which the background and the aerial perspective draw, are blended between day, dusk and night by the
// sun's height, and by day the haze whitens the horizon towards the colour it scatters and pales the zenith.
export function setSkyLight(light: SkyLight, sun: Point, altitude: number, air: HazeAir, optics: AirOptics) {
  const height = sun[1];
  const days = smooth(-0.02, 0.3, height);
  const nights = 1 - smooth(-0.24, -0.04, height);
  const dusks = Math.max(0, 1 - days - nights);
  light.skySun.value.fromArray(sun).normalize();
  blend(light.skyZenith.value, DaySky.zenith, DuskSky.zenith, NightSky.zenith, days, dusks, nights);
  blend(light.skyHorizon.value, DaySky.horizon, DuskSky.horizon, NightSky.horizon, days, dusks, nights);
  blend(light.skyGlow.value, DaySky.glow, DuskSky.glow, NightSky.glow, days * 0.25, dusks, 0);
  const horizon = light.skyHorizon.value;
  const whitening = days * Math.max(0, hazeShare(air, altitude, optics) - ReferenceHaze) / (1 - ReferenceHaze);
  // The horizon looks through haze too deep to see past, which sends back light it has scattered many times:
  // of white sunlight, each colour as a deep layer of particles with that colour's single-scattering albedo
  // reflects it (Hapke's diffusive reflectance, 1981: (1 − γ) / (1 + γ) with γ = √(1 − albedo)), kept at the
  // horizon's brightness. A little absorption per scattering adds up over many: dust is tan.
  const grey = 0.2126 * horizon.r + 0.7152 * horizon.g + 0.0722 * horizon.b;
  for (let colour = 0; colour < 3; colour++) {
    const gamma = Math.sqrt(Math.max(0, 1 - air.albedo[colour]));
    tint[colour] = (1 - gamma) / (1 + gamma);
  }
  const tintGrey = 0.2126 * tint[0] + 0.7152 * tint[1] + 0.0722 * tint[2];
  horizon.lerp(scratch.setRGB(grey * tint[0] / tintGrey, grey * tint[1] / tintGrey, grey * tint[2] / tintGrey), whitening);
  light.skyZenith.value.lerp(horizon, 1 - Math.exp(-ZenithPaling * days * Math.max(0, hazeAbove(air, altitude) - ReferenceDepth)));

  const sky = clearSky(light.skySun.value.y, altitude, air, optics);
  light.sunLight.value.setRGB(...sky.direct).multiplyScalar(SunStrength);
  // What the sun's light leaves in the air, down to the night sky's own (a blend of its zenith and horizon), which
  // the twilight reaches with the sun about 1.5° down.
  const night = light.skyAmbient.value.set(NightSky.zenith).lerp(scratch.set(NightSky.horizon), 0.55).multiplyScalar(NightLight);
  night.setRGB(
    Math.max(night.r, sky.diffuse[0] * SunStrength), Math.max(night.g, sky.diffuse[1] * SunStrength), Math.max(night.b, sky.diffuse[2] * SunStrength),
  );
  // The dome's colours are still a palette rather than the radiance of that air, so the clouds drawn against them
  // take the dome's light as drawn; lit by the air's, they would stand black against a bright dusk.
  light.domeLight.value.copy(light.skyZenith.value).lerp(light.skyHorizon.value, 0.55).multiplyScalar(DomeLight.day * days + DomeLight.dusk * dusks + DomeLight.night * nights);
}
