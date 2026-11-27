import { abs, cameraPosition, exp, float, length, log, max, mix, normalize, select, smoothstep, vec3 } from "three/tsl";
import type { Node, Vector4 } from "three/webgpu";
import { HazeEdge, VeilHeight, VeilReach } from "../constant/air";
import type { SkyNodes } from "../type/SkyNodes";
import { skyRadiance } from "./radiance";

type Vec3 = Node<"vec3">;
type Float = Node<"float">;

// The parts of the air's paths that end at the eye, the same for every pixel of a view, and so worked out once a
// view (SkyNodes.airEye) as airMean and bandMean would: the air's density at the eye against the sea's, for the
// veil and for the clean air, the haze's ln(1 + e^x) at the eye, and its share of the eye in the haze. `from` is
// the eye's height above the sea, `top` the haze band's top and `clean` the metres over which the clean air thins.
export function eyeAir(from: number, top: number, clean: number, into: Vector4) {
  const floor = Math.max(from, -2000);
  const edge = (top - from) / HazeEdge;
  return into.set(Math.exp(-floor / VeilHeight), Math.exp(-floor / clean), Math.max(edge, 0) + Math.log(Math.exp(-Math.abs(edge)) + 1),
    1 / (Math.exp(Math.min(-edge, 80)) + 1));
}

// Mean density, against the sea's, along a straight path between two heights of air that thins by e every
// `scale` metres; `low` is the density at `from`.
function airMean(low: Float, from: Float, to: Float, scale: Float | number) {
  const high = exp(max(to, -2000).div(scale).negate());
  const climb = max(to, -2000).sub(max(from, -2000)).div(scale);
  const level = abs(climb).lessThan(0.01);
  return select(level, low.mul(climb.mul(-0.5).add(1)), low.sub(high).div(select(level, float(1), climb)));
}

// ln(1 + e^x) without overflow.
function softplus(x: Float) {
  return max(x, 0).add(log(exp(abs(x).negate()).add(1)));
}

// Share of a straight path between two heights that lies in the haze below `top`, whose edge fades over HazeEdge;
// `start` is softplus((top - from) / HazeEdge), `within` the share of `from` in the haze (eyeAir).
function bandMean(start: Float, within: Float, from: Float, to: Float, top: Float) {
  const climb = to.sub(from);
  const level = abs(climb).lessThan(1);
  const inside = start.sub(softplus(top.sub(to).div(HazeEdge))).mul(HazeEdge).div(select(level, float(1), climb));
  return select(level, within, inside);
}

// What the sky sends from a direction to something `range` metres off: the clear sky's radiance, or near the eye
// under cloud that cloud's grey.
export function airLight(sky: SkyNodes, direction: Vec3, range: Float) {
  return mix(skyRadiance(sky, direction), sky.airVeil.xyz, sky.airVeil.w.mul(smoothstep(VeilReach[0], VeilReach[1], range).oneMinus()));
}

// Aerial perspective: what distance does to a colour seen through the air. Light is lost along the way and the
// sky's own light scattered in, blue first. The clean air thins with height; the weather's haze fills its low
// band, taking each colour by its spectrum, and the veil, spread from the sea, thins like the air. What comes in
// is the sky towards the horizon, the light of air too deep to see past, which already holds what the haze
// absorbs (skyLight's horizon); taking it off again here would count it twice. At the end of the view the ground
// has become the clear sky just above the horizon in its direction, which is what the background draws there, so
// the terrain never ends in an edge.
export function airFade(sky: SkyNodes, color: Vec3, fromEye: Vec3) {
  const range = length(fromEye);
  const from = cameraPosition.y.sub(sky.airSea);
  const to = from.add(fromEye.y);
  const fine = sky.airHazeSpectrum.mul(bandMean(sky.airEye.z, sky.airEye.w, from, to, sky.airHaze.y).mul(sky.airHaze.x));
  const veil = airMean(sky.airEye.x, from, to, VeilHeight).mul(sky.airHaze.z.div(VeilHeight));
  const clean = sky.airClean.xyz.mul(airMean(sky.airEye.y, from, to, sky.airClean.w));
  const depth = clean.add(fine).add(veil).mul(range).mul(sky.airShare);
  const lost = max(exp(depth.negate()).oneMinus(), smoothstep(sky.airEnd.x, sky.airEnd.y, range));
  // Just over the horizon the way the ground lies, also for ground straight below.
  const low = normalize(vec3(fromEye.x.add(1e-3), max(fromEye.y, 0).mul(0.4).add(length(fromEye.xz).mul(0.03)), fromEye.z));
  return mix(color, airLight(sky, low, range), lost);
}
