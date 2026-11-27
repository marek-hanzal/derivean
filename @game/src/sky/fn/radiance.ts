import { dot, exp, max, mix, positionWorldDirection, smoothstep, step } from "three/tsl";
import type { Node } from "three/webgpu";
import type { SkyNodes } from "../type/SkyNodes";

// Radiance of the clear sky in a direction: zenith over horizon, the warm band on the low sun's side, and the
// aureole around it.
export function skyRadiance(sky: SkyNodes, direction: Node<"vec3">) {
  const up = max(direction.y, 0);
  const toSun = dot(direction, sky.skySun);
  // The band's fourth power and the aureole's thirty-second by squaring.
  const squared = (value: Node<"float">, times: number): Node<"float"> => (times ? squared(value.mul(value), times - 1) : value);
  const band = squared(max(toSun.mul(0.5).add(0.5), 0), 2);
  const near = squared(max(toSun, 0), 5);
  return mix(sky.skyHorizon, sky.skyZenith, exp(up.mul(-3.4)).oneMinus())
    .add(sky.skyGlow.mul(exp(up.mul(-6))).mul(band).mul(0.8))
    .add(sky.skyGlow.mul(near).mul(0.5));
}

// The scene's background: the clear sky and the sun's disc.
export function skyBackground(sky: SkyNodes) {
  const direction = positionWorldDirection.normalize();
  const disc = smoothstep(0.9999, 0.99996, dot(direction, sky.skySun)).mul(step(0, direction.y.add(0.02)));
  return skyRadiance(sky, direction).add(sky.sunDisc.mul(disc).mul(14));
}
