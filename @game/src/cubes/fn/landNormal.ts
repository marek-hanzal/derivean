import { If, float, floor, int, length, normalize, select, smoothstep, vec2, vec3 } from "three/tsl";
import type { Node } from "three/webgpu";
import { HeightSide, LandEdges, LandReach, LandShare, NoHeight, Steep } from "../constant/land";
import type { LevelHeights } from "../service/LevelHeights";

type Float = Node<"float">;

// The land a coarse cube stands for, and how much of a face's light is that land's.
//
// A coarse cube is the voxels within it, and a step between two of them is mostly a slope cut into a stair: seen
// from far enough for the voxels to fall within a pixel, that stair is not a wall but the slope's own light, as
// much of it on the riser as on the tops beside it. So a face of a coarse cube takes part of its light as the
// slope of the land round its cube, measured on the level's own column heights `LandReach` columns to either
// side. The cube keeps its own face, edges and corners, and the rest of its light by its own normal, so a step
// still shows; where the land is steeper than the engine keeps loose cover on, it is rock standing as a face,
// and its cubes keep all their own light. Each face takes one value for its whole cube: nothing is blended
// across a face.
//
// `inside` is a point within the fragment's cube, render space; `level` and `edge` are its level's. Call it inside
// a node function.
export function landNormal(heights: LevelHeights["nodes"], level: Node<"uint">, edge: Float, inside: Node<"vec3">) {
  const share = float(0).toVar();
  const normal = vec3(0, 1, 0).toVar();
  const place = heights.places.element(level);
  If(place.w.greaterThan(0).and(edge.greaterThan(LandEdges[0])), () => {
    const column = floor(inside.xz.div(edge).add(place.xy));
    const read = (dx: number, dz: number) => {
      const x = int(column.x).add(dx).bitAnd(HeightSide - 1);
      const z = int(column.y).add(dz).bitAnd(HeightSide - 1);
      const block = z.shiftRight(5).mul(HeightSide / 32).add(x.shiftRight(5));
      return heights.ring.element(int(place.z).add(block.mul(1024)).add(z.bitAnd(31).mul(32)).add(x.bitAnd(31)));
    };
    const own = read(0, 0).toVar();
    If(own.greaterThan(NoHeight / 2), () => {
      // A column not held reads as the cube's own, so the slope is measured on what is there.
      const held = (dx: number, dz: number) => {
        const height = read(dx, dz);
        return select(height.greaterThan(NoHeight / 2), height, own);
      };
      const across = (along: "x" | "z") => {
        let rise: Float = float(0);
        for (const side of [-1, 0, 1]) {
          const ahead = along === "x" ? held(LandReach, side) : held(side, LandReach);
          const behind = along === "x" ? held(-LandReach, side) : held(side, -LandReach);
          rise = rise.add(ahead.sub(behind));
        }
        return rise.div(3 * 2 * LandReach);
      };
      const slope = vec2(across("x"), across("z"));
      const steepness = length(slope);
      normal.assign(normalize(vec3(slope.x.negate(), 1, slope.y.negate())));
      share.assign(smoothstep(LandEdges[0], LandEdges[1], edge).mul(smoothstep(Steep[0], Steep[1], steepness).oneMinus()).mul(LandShare));
    });
  });
  return { normal, share };
}
