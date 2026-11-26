import { FaceWords, Upright } from "../constant/layout";
import { packFace, readFace } from "./packFaces";

// A square of ground in cubes from a batch's anchor: x from x0 up to x1, z from z0 up to z1.
export interface Cut { x0: number; x1: number; z0: number; z1: number }

// Cubes a cap hangs down from the top it closes.
const Depth = 8;

// The faces that close a level where a finer level's square is cut out of it. The level's ground is solid from
// its tops down, so along the square's sides it stands open like a cut cake wherever it is higher than the finer
// ground beside it. Under every top that reaches a side from outside, a riser on that side faces into the
// square: the cross-section of the level's own ground, made of what the top is made of. `runs` are the chunks'
// runs in `packed` with the chunk's place in cubes, `side` cubes along a chunk's edge; `cut` lies on the level's
// lattice.
export function capFaces(packed: Uint32Array, runs: Iterable<{ at: number; count: number; home: readonly number[] }>, cut: Cut, level: number,
  side: number) {
  const out: number[] = [];
  const riser = (axis: number, negative: boolean, plane: number, lowU: number, lowV: number, sizeU: number, sizeV: number, material: number) => {
    out.length += FaceWords;
    packFace(out, out.length - FaceWords, axis, negative, plane, lowU, lowV, sizeU, sizeV, material, 0, level, 0, true);
  };
  for (const { at, count, home: [x, , z] } of runs) {
    const besideX = (x < cut.x0 && x + side >= cut.x0) || (x <= cut.x1 && x + side > cut.x1);
    const besideZ = (z < cut.z0 && z + side >= cut.z0) || (z <= cut.z1 && z + side > cut.z1);
    if (!(besideX && z < cut.z1 && z + side > cut.z0) && !(besideZ && x < cut.x1 && x + side > cut.x0)) continue;
    for (let face = at * FaceWords; face < (at + count) * FaceWords; face += FaceWords) {
      const { axis, negative, plane: top, lowU: z0, lowV: x0, sizeU, sizeV, material } = readFace(packed, face);
      if (axis !== Upright || negative) continue;
      const z1 = z0 + sizeU;
      const x1 = x0 + sizeV;
      const zLow = Math.max(z0, cut.z0);
      const zHigh = Math.min(z1, cut.z1);
      if (zHigh > zLow) {
        if (x0 < cut.x0 && x1 >= cut.x0) riser(0, false, cut.x0, top - Depth, zLow, Depth, zHigh - zLow, material);
        if (x0 <= cut.x1 && x1 > cut.x1) riser(0, true, cut.x1, top - Depth, zLow, Depth, zHigh - zLow, material);
      }
      const xLow = Math.max(x0, cut.x0);
      const xHigh = Math.min(x1, cut.x1);
      if (xHigh > xLow) {
        if (z0 < cut.z0 && z1 >= cut.z0) riser(2, false, cut.z0, xLow, top - Depth, xHigh - xLow, Depth, material);
        if (z0 <= cut.z1 && z1 > cut.z1) riser(2, true, cut.z1, xLow, top - Depth, xHigh - xLow, Depth, material);
      }
    }
  }
  return out;
}
