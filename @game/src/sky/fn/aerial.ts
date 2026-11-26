import { abs, cameraPosition, exp, float, length, max, mix, normalize, select, smoothstep, vec3 } from "three/tsl";
import { CleanAir, CleanAirHeight, HazeAir, HazeAirHeight, HighHaze, HighHazeHeight, VeilReach } from "../constant/air";
import type { SkyNodes } from "../type/SkyNodes";
import type { Node } from "three/webgpu";
import { skyRadiance } from "./radiance";

type Vec3 = Node<"vec3">;
type Float = Node<"float">;

// Mean density, against the sea's, along a straight path between two heights of air that thins by e every
// `scale` metres.
function airMean(from: Float, to: Float, scale: number) {
  const low = exp(max(from, -2000).div(-scale));
  const high = exp(max(to, -2000).div(-scale));
  const climb = max(to, -2000).sub(max(from, -2000)).div(scale);
  const level = abs(climb).lessThan(0.01);
  return select(level, low.mul(climb.mul(-0.5).add(1)), low.sub(high).div(select(level, float(1), climb)));
}

// What the sky sends from a direction to something `range` metres off: the clear sky's radiance, or near the eye
// under cloud that cloud's grey.
export function airLight(sky: SkyNodes, direction: Vec3, range: Float) {
  return mix(skyRadiance(sky, direction), sky.airVeil.xyz, sky.airVeil.w.mul(smoothstep(VeilReach[0], VeilReach[1], range).oneMinus()));
}

// Aerial perspective: what distance does to a colour seen through the air. Light is lost along the way and the
// sky's own light scattered in, blue first. The clean air and the haze in it thin with height. At the end of
// the view the ground has become the clear sky just above the horizon in its direction, which is what the
// background draws there, so the terrain never ends in an edge.
export function airFade(sky: SkyNodes, color: Vec3, fromEye: Vec3) {
  const range = length(fromEye);
  const from = cameraPosition.y.sub(sky.airSea);
  const to = from.add(fromEye.y);
  const depth = vec3(...CleanAir).mul(airMean(from, to, CleanAirHeight)).add(airMean(from, to, HazeAirHeight).mul(HazeAir))
    .add(airMean(from, to, HighHazeHeight).mul(HighHaze / HighHazeHeight)).mul(range).mul(sky.airShare);
  const lost = max(exp(depth.negate()).oneMinus(), smoothstep(sky.airEnd.x, sky.airEnd.y, range));
  // Just over the horizon the way the ground lies, also for ground straight below.
  const low = normalize(vec3(fromEye.x.add(1e-3), max(fromEye.y, 0).mul(0.4).add(length(fromEye.xz).mul(0.03)), fromEye.z));
  return mix(color, airLight(sky, low, range), lost);
}
