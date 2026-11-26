import { Fn, abs, float, max, min, mix, select, smoothstep, step, texture, vec2, vec4 } from "three/tsl";
import type { Node } from "three/webgpu";
import { EdgeFade } from "../constant/cascades";
import type { ShadowNodes } from "../type/ShadowNodes";

type Float = Node<"float">;

// Metres along a ray until it leaves a span from `low` to `high`, for a point at `at` within it; `towards` is the
// ray's share along that axis.
const leaves = (towards: Float, low: Float, high: Float, at: Float) =>
  select(towards.greaterThanEqual(0), high.sub(at).div(max(towards, 1e-4)), low.sub(at).div(min(towards, -1e-4)));

// Share of the sun's light that reaches a point in render space, 0 in shadow. A map holds the faces that look
// away from the sun, so a lit face never shadows itself and needs no slope bias: the point is lit where it lies
// nearer the sun than the first such face on its ray.
//
// A point takes the shadows of the cascade whose square of ground it lies in, cast by cubes of its own size, and
// from every coarser cascade only what lies beyond that square: there the coarser map holds the same ground
// again as larger cubes, which would shadow it a second time and elsewhere. So the point is moved along its ray
// to where the ray leaves the square before it is looked up in the next map.
export function sunlit(shadow: ShadowNodes, position: Node<"vec3">): Float {
  return Fn(() => {
    const lit = float(1).toVar();
    const answered = float(0).toVar();
    shadow.maps.forEach((map, index) => {
      const ray = shadow.rays.element(index);
      const square = shadow.squares.element(index);
      const clip = shadow.matrices.element(index).mul(vec4(position, 1));
      const within = step(square.x, position.x).mul(step(position.x, square.z)).mul(step(square.y, position.z)).mul(step(position.z, square.w));
      const held = smoothstep(1 - EdgeFade, 1, max(abs(clip.x), abs(clip.y))).oneMinus().mul(step(0, clip.z)).mul(step(clip.z, 1)).mul(within);
      const depth = clip.z.add(answered.mul(ray.w));
      lit.mulAssign(mix(1, texture(map, vec2(clip.x.mul(0.5).add(0.5), clip.y.mul(-0.5).add(0.5))).compare(depth), held));
      const out = min(leaves(ray.x, square.x, square.z, position.x), leaves(ray.z, square.y, square.w, position.z));
      answered.assign(max(answered, out.mul(held)));
    });
    return lit;
  })();
}
