import { type InstancedBufferAttribute, MeshBasicNodeMaterial, type Node, Vector3 } from "three/webgpu";
import { cross, float, fract, instancedBufferAttribute, length, max, positionGeometry, sin, smoothstep, uniform, varying, vec3 } from "three/tsl";
import type { SkyNodes } from "../../sky/type/SkyNodes";
import { FallBox, type Fall } from "../constant/fall";

// Drops as camera-facing streaks along their velocity, in a mesh that stands at the eye. Each lives at a fixed
// place in a cube of world space that repeats, falls through it, and is drawn where it wraps nearest the eye, so
// the rain stays put when the eye moves instead of following it.
export function fallMaterial(fall: Fall, sky: SkyNodes, seeds: InstancedBufferAttribute) {
  const material = new MeshBasicNodeMaterial({ transparent: true, depthWrite: false, fog: false });
  // Where the eye is in the cube, and how far the drops have fallen through it, both in cubes.
  const eyeInBox = uniform(new Vector3());
  const fallen = uniform(new Vector3());
  const velocity = uniform(new Vector3(0, -fall.speed, 0));
  const seconds = uniform(0);
  const seed = instancedBufferAttribute(seeds, "vec3") as Node<"vec3">;
  const centre = fract(seed.add(fallen).sub(eyeInBox)).sub(0.5).mul(FallBox).toVar();
  const swayed = vec3(centre.x.add(sin(seconds.mul(1.3).add(seed.y.mul(40))).mul(fall.sway)), centre.y,
    centre.z.add(sin(seconds.mul(1.3).add(seed.x.mul(40))).mul(fall.sway)));
  // A drop is as long as it travels in its exposure and never shorter than it is wide.
  const streak = velocity.mul(fall.trail);
  const trail = streak.mul(max(float(1), float(fall.width).div(length(streak))));
  const side = cross(trail, swayed.negate()).normalize().mul(fall.width);
  material.positionNode = swayed.add(trail.mul(positionGeometry.y.sub(0.5))).add(side.mul(positionGeometry.x));
  const range = length(swayed);
  const fade = varying(smoothstep(FallBox * 0.3, FallBox * 0.5, range).oneMinus().mul(smoothstep(0.4, 1.6, range)));
  material.colorNode = sky.skyLight.mul(0.4).add(sky.sunLight.mul(0.06));
  material.opacityNode = fade.mul(fall.opacity);
  return { material, eyeInBox, fallen, velocity, seconds };
}
