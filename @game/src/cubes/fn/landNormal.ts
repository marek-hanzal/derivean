import { If, float, floor, int, ivec2, length, log2, normalize, select, smoothstep, uint, vec3 } from "three/tsl";
import type { Node } from "three/webgpu";
import { HeightBlocks, LandEdges, LandShare, NoHeight, SnowDepth, Steep } from "../constant/land";
import { LevelRows } from "../constant/levels";
import type { LevelHeights } from "../service/LevelHeights";
import type { LevelTable } from "../type/LevelTable";

type Float = Node<"float">;

// No slope measured.
const Unmeasured = 1e6;

// The land a cube stands on, and how much of a face's light is that land's.
//
// A coarse cube is the voxels within it, and a step between two of them is mostly a slope cut into a stair: seen
// from far enough for the voxels to fall within a pixel, that stair is not a wall but the slope's own light, as
// much of it on the riser as on the tops beside it. So a face of a coarse cube takes part of its light as the
// slope of the land round its cube, measured on the level's own column heights, and as the sun reaches the top
// of that column (`litAt`, a point in render space): a riser that turns from the sun is still on a slope the sun
// lights. The cube keeps its own faces, edges and corners and the rest of its light by its own normal, so a step
// still shows; where the land is steeper than the engine keeps loose cover on, it is rock standing as a face,
// and its cubes keep all their own light. Every fragment of a cube takes the same slope: nothing is blended
// across a face. The slopes are measured as the heights come (landSlopes), one read a fragment.
//
// `steep` is the slope, rise over run, across the near reach alone, for the snow on risers: a face finer than the
// finest level that keeps heights is measured on that level, which is whole under the finer ground. It is huge
// where nothing was measured: where no heights are held, where the cube lies more than SnowDepth cubes of that
// level below its column's top, and where the face neither takes land light nor `wants` the slope.
//
// `inside` is a point within the fragment's cube, render space; `level` and `edge` are its level's. A top, where
// `top` is true, takes the sun as the fragment itself does, `faceLit`: it mostly is the top of its column, and one
// that is not (under an overhang, in a pit) is better lit as itself. So a top's land light follows a shadow across
// it; other faces take the sun at the top of the column. Call it inside a node function.
export function landNormal(heights: LevelHeights["nodes"], table: LevelTable, level: Node<"uint">, edge: Float,
  inside: Node<"vec3">, litAt: (point: Node<"vec3">) => Float, top: Node<"bool">, faceLit: Float, wants?: Node<"bool">) {
  const share = float(0).toVar();
  const normal = vec3(0, 1, 0).toVar();
  const lit = float(1).toVar();
  const steep = float(Unmeasured).toVar();
  const lights = edge.greaterThan(LandEdges[0]);
  const measured = select(level.lessThan(uint(heights.finest)), uint(heights.finest), level);
  const span = table.node.element(int(measured).mul(LevelRows)).w;
  const place = heights.places.element(measured);
  const first = heights.firsts.element(measured);
  If(first.greaterThanEqual(0).and(wants ? lights.or(wants) : lights), () => {
    // Columns from the corner of the square the render origin is in.
    const column = floor(inside.xz.div(span).add(place.xy));
    const at = ivec2(int(column.x), int(column.y));
    const square = ivec2(int(place.z).add(at.x.shiftRight(5)), int(place.w).add(at.y.shiftRight(5)));
    const spot = int(first).add(square.y.bitAnd(HeightBlocks - 1).mul(HeightBlocks)).add(square.x.bitAnd(HeightBlocks - 1));
    const ours = heights.owners.element(spot.mul(2)).equal(float(square.x)).and(heights.owners.element(spot.mul(2).add(1)).equal(float(square.y)));
    // Height, slope across the near reach, slope across both (landSlopes).
    const land = heights.columns.element(spot.mul(1024).add(at.y.bitAnd(31).mul(32)).add(at.x.bitAnd(31))).toVar();
    const own = select(ours, land.x, float(NoHeight)).toVar();
    If(own.greaterThan(NoHeight / 2), () => {
      // Whole cubes of the measured level between the cube and the top of its column: only the step at the
      // surface is the slope's stair, not a wall further down (a pit, a cave, the cut of a level, a cliff).
      const below = own.sub(1).sub(floor(inside.y.add(heights.lift).div(span)));
      steep.assign(select(below.greaterThan(SnowDepth), float(Unmeasured), land.y));
      If(lights, () => {
        const slope = land.zw;
        normal.assign(normalize(vec3(slope.x.negate(), 1, slope.y.negate())));
        share.assign(smoothstep(Math.log2(LandEdges[0]), Math.log2(LandEdges[1]), log2(edge))
          .mul(smoothstep(Steep[0], Steep[1], length(slope)).oneMinus()).mul(LandShare));
        // The top of the column, a fiftieth of a cube up, in render space.
        If(top, () => { lit.assign(faceLit); }).Else(() => {
          const middle = column.add(0.5).sub(place.xy).mul(span);
          lit.assign(litAt(vec3(middle.x, own.add(0.02).mul(span).sub(heights.lift), middle.y)));
        });
      });
    });
  });
  return { normal, share, lit, steep };
}
