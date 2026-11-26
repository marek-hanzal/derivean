import { dot, exp, max, mix, positionWorldDirection, pow, smoothstep, step, vec3 } from "three/tsl";
import type { Node } from "three/webgpu";
import type { SkyNodes } from "../type/SkyNodes";

// Radiance of the clear sky in a direction: zenith over horizon, the warm band on the low sun's side, and the
// aureole around it.
export function skyRadiance(sky: SkyNodes, direction: Node<"vec3">) {
  const up = max(direction.y, 0);
  const toSun = dot(direction, sky.skySun);
  return mix(sky.skyHorizon, sky.skyZenith, exp(up.mul(-3.4)).oneMinus())
    .add(sky.skyGlow.mul(exp(up.mul(-6))).mul(pow(max(toSun.mul(0.5).add(0.5), 0), 4)).mul(0.8))
    .add(sky.skyGlow.mul(pow(max(toSun, 0), 32)).mul(0.5));
}

// The scene's background: the clear sky and the sun's disc.
export function skyBackground(sky: SkyNodes) {
  const direction = positionWorldDirection.normalize();
  const disc = smoothstep(0.9999, 0.99996, dot(direction, sky.skySun)).mul(step(0, direction.y.add(0.02)));
  return skyRadiance(sky, direction).add(sky.sunDisc.mul(disc).mul(14));
}
