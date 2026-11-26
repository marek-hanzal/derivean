import { If, float, floor, int, ivec2, length, log2, normalize, select, smoothstep, vec2, vec3 } from "three/tsl";
import type { Node } from "three/webgpu";
import { HeightBlocks, LandEdges, LandReach, LandShare, NoHeight, Steep } from "../constant/land";
import type { LevelHeights } from "../service/LevelHeights";

type Float = Node<"float">;

// The land a coarse cube stands for, and how much of a face's light is that land's.
//
// A coarse cube is the voxels within it, and a step between two of them is mostly a slope cut into a stair: seen
// from far enough for the voxels to fall within a pixel, that stair is not a wall but the slope's own light, as
// much of it on the riser as on the tops beside it. So a face of a coarse cube takes part of its light as the
// slope of the land round its cube, measured on the level's own column heights, and as the sun reaches the top
// of that column (`litAt`, a point in render space): a riser that turns from the sun is still on a slope the sun
// lights. The cube keeps its own faces, edges and corners and the rest of its light by its own normal, so a step
// still shows; where the land is steeper than the engine keeps loose cover on, it is rock standing as a face,
// and its cubes keep all their own light. Every fragment of a cube takes the same value: nothing is blended
// across a face.
//
// `inside` is a point within the fragment's cube, render space; `level` and `edge` are its level's. Call it inside
// a node function.
export function landNormal(heights: LevelHeights["nodes"], level: Node<"uint">, edge: Float, inside: Node<"vec3">,
  litAt: (point: Node<"vec3">) => Float) {
  const share = float(0).toVar();
  const normal = vec3(0, 1, 0).toVar();
  const lit = float(1).toVar();
  const place = heights.places.element(level);
  const first = heights.firsts.element(level);
  If(first.greaterThanEqual(0).and(edge.greaterThan(LandEdges[0])), () => {
    // Columns from the corner of the square the render origin is in.
    const column = floor(inside.xz.div(edge).add(place.xy));
    const read = (dx: number, dz: number) => {
      const at = ivec2(int(column.x).add(dx), int(column.y).add(dz));
      const square = ivec2(int(place.z).add(at.x.shiftRight(5)), int(place.w).add(at.y.shiftRight(5)));
      const spot = int(first).add(square.y.bitAnd(HeightBlocks - 1).mul(HeightBlocks)).add(square.x.bitAnd(HeightBlocks - 1));
      const ours = heights.owners.element(spot.mul(2)).equal(float(square.x)).and(heights.owners.element(spot.mul(2).add(1)).equal(float(square.y)));
      const height = heights.ring.element(spot.mul(1024).add(at.y.bitAnd(31).mul(32)).add(at.x.bitAnd(31)));
      return select(ours, height, float(NoHeight));
    };
    const own = read(0, 0).toVar();
    If(own.greaterThan(NoHeight / 2), () => {
      // A column not held reads as the cube's own, so the slope is measured on what is there.
      const held = (dx: number, dz: number) => {
        const height = read(dx, dz);
        return select(height.greaterThan(NoHeight / 2), height, own);
      };
      const across = (along: "x" | "z", reach: number) => {
        let rise: Float = float(0);
        for (const side of [-1, 0, 1]) {
          const ahead = along === "x" ? held(reach, side) : held(side, reach);
          const behind = along === "x" ? held(-reach, side) : held(side, -reach);
          rise = rise.add(ahead.sub(behind));
        }
        return rise.div(3 * 2 * reach);
      };
      const slope = vec2(across("x", LandReach[0]).add(across("x", LandReach[1])), across("z", LandReach[0]).add(across("z", LandReach[1]))).mul(0.5);
      normal.assign(normalize(vec3(slope.x.negate(), 1, slope.y.negate())));
      share.assign(smoothstep(Math.log2(LandEdges[0]), Math.log2(LandEdges[1]), log2(edge))
        .mul(smoothstep(Steep[0], Steep[1], length(slope)).oneMinus()).mul(LandShare));
      // The top of the column, a fiftieth of a cube up, in render space.
      const middle = column.add(0.5).sub(place.xy).mul(edge);
      lit.assign(litAt(vec3(middle.x, own.add(0.02).mul(edge).sub(heights.lift), middle.y)));
    });
  });
  return { normal, share, lit };
}
