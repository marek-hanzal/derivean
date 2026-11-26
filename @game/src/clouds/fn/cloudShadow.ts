import { Fn, float, max, min, mix, smoothstep, texture, vec2 } from "three/tsl";
import type { Node } from "three/webgpu";
import { ShadowSize } from "../constant/shadow";
import type { CloudShade } from "../type/CloudShade";

const Tap = 0.75 / ShadowSize;

// Share of direct sunlight a render-space point receives through the clouds. The point is carried back along the
// sun's rays to the map's plane, so a shadow lies where the cloud that casts it stands in the way, at any height
// of ground. Beyond the map the ground takes the cloud over the eye.
export function cloudShadow(shade: CloudShade, position: Node<"vec3">): Node<"float"> {
  return Fn(() => {
    const towardsSun = shade.sun;
    const place = shade.place;
    const foot = position.xz.sub(towardsSun.xz.mul(position.y.sub(place.w).div(max(towardsSun.y, 0.12))));
    const at = foot.sub(place.xy).div(max(place.z, 1)).add(0.5);
    const edge = min(at, at.oneMinus());
    // Four taps round the point: a shadow's edge is softer than a texel of the map.
    const tap = (x: number, y: number) => texture(shade.map, at.add(vec2(x, y))).level(float(0)).r;
    const lit = tap(Tap, Tap).add(tap(-Tap, Tap)).add(tap(Tap, -Tap)).add(tap(-Tap, -Tap));
    const drawn = place.z.greaterThan(0).and(towardsSun.y.greaterThan(0));
    return drawn.select(mix(shade.beyond, lit.mul(0.25), smoothstep(0, 0.06, min(edge.x, edge.y))), float(1));
  })();
}
