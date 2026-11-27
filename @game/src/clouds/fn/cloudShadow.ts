import { Fn, float, max, min, mix, smoothstep, texture, vec2 } from "three/tsl";
import type { Node, Texture, UniformNode, Vector4 } from "three/webgpu";
import { ShadowSize } from "../constant/shadow";
import type { CloudShade } from "../type/CloudShade";

const Tap = 0.75 / ShadowSize;

// Share of direct sunlight a render-space point receives through the clouds. The point is carried back along the
// sun's rays to a map's plane, so a shadow lies where the cloud that casts it stands in the way, at any height of
// ground. The near map is read where it reaches, fading into the far one towards its border (as cascaded shadow
// maps hand over, Dimitrov 2007, docs/credits/rendering.md); beyond the far map the ground takes the cloud over
// the eye.
export function cloudShadow(shade: CloudShade, position: Node<"vec3">): Node<"float"> {
  return Fn(() => {
    const towardsSun = shade.sun;
    // What a map lets through at the point, and how much of the point the map holds: 1 inside, falling to 0 at
    // its border.
    const read = (map: Texture, place: UniformNode<"vec4", Vector4>) => {
      const foot = position.xz.sub(towardsSun.xz.mul(position.y.sub(place.w).div(max(towardsSun.y, 0.12))));
      const at = foot.sub(place.xy).div(max(place.z, 1)).add(0.5);
      const edge = min(at, at.oneMinus());
      // Four taps round the point: a shadow's edge is softer than a texel of the map (filtered as Reeves et al. 1987 do).
      const tap = (x: number, y: number) => texture(map, at.add(vec2(x, y))).level(float(0)).r;
      const lit = tap(Tap, Tap).add(tap(-Tap, Tap)).add(tap(Tap, -Tap)).add(tap(-Tap, -Tap)).mul(0.25);
      return { lit, held: smoothstep(0, 0.06, min(edge.x, edge.y)).mul(place.z.greaterThan(0).select(float(1), float(0))) };
    };
    const near = read(shade.maps[0], shade.places[0]);
    const far = read(shade.maps[1], shade.places[1]);
    const lit = mix(mix(shade.beyond, far.lit, far.held), near.lit, near.held);
    return towardsSun.y.greaterThan(0).and(shade.places[1].z.greaterThan(0)).select(lit, float(1));
  })();
}
