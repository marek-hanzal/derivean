import type { Point } from "../../../contract/GameBridge";
import { type Box, Nowhere, column, snapped } from "./levelTable";

const holds = (outer: Box, inner: Box) => outer.min[0] <= inner.min[0] && outer.min[2] <= inner.min[2]
  && outer.max[0] >= inner.max[0] && outer.max[2] >= inner.max[2];

function shared(a: Box, b: Box): Box {
  const min: Point = [Math.max(a.min[0], b.min[0]), a.min[1], Math.max(a.min[2], b.min[2])];
  const max: Point = [Math.min(a.max[0], b.max[0]), a.max[1], Math.min(a.max[2], b.max[2])];
  return min[0] < max[0] && min[2] < max[2] ? { min, max } : Nowhere;
}

// The square of ground each level is drawn in, from the finest; `levels` come in that order with the box the
// engine gives them. A level holds whole columns of chunks, so a box counts by its footprint at any height.
// Boxes need not be nested: the voxels' box is not tied to the cubes', and a level that is short of chunks shares
// a shrunken box with a finer one. So a level whose box does not hold the box of the last finer level drawn is
// left out, and the next coarser one shows there. A square ends on the lattice of the next coarser level drawn,
// inside its own box, where the level is whole, and inside every coarser square: each level then draws its
// square less the next finer one, the two meet on whole cubes of the coarser, and no ground is drawn twice.
export function levelSquares(levels: readonly { level: number; edge: number; box: Box | null }[]) {
  const drawn: { level: number; edge: number; box: Box }[] = [];
  for (const { level, edge, box } of levels) {
    if (box && (!drawn.length || holds(box, drawn[drawn.length - 1].box))) drawn.push({ level, edge, box });
  }
  const squares = drawn.map(({ level, edge, box }, index) =>
    ({ level, edge, own: index + 1 < drawn.length ? snapped(column(box), drawn[index + 1].edge) : column(box) }));
  for (let index = squares.length - 2; index >= 0; index--) squares[index].own = shared(squares[index].own, squares[index + 1].own);
  return squares;
}
