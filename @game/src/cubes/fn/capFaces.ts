import { packFace } from "./packFaces";

// A square of ground in cubes from a batch's anchor: x and z from `low` up to `high`.
export interface Cut { x0: number; x1: number; z0: number; z1: number }

// Cubes a cap hangs down from the top it closes.
const Depth = 8;
// Cubes along the edge of a chunk.
const Side = 32;

const short = (word: number) => (word & 0xffff) << 16 >> 16;

// The faces that close a level where a finer level's square is cut out of it. The level's ground is solid from
// its tops down, so along the square's sides it stands open like a cut cake wherever it is higher than the finer
// ground beside it. Under every top that reaches a side from outside, a riser on that side faces into the
// square: the cross-section of the level's own ground, made of what the top is made of. `runs` are the chunks'
// runs in `packed` with the chunk's place in cubes; `cut` lies on the level's lattice.
export function capFaces(packed: Uint32Array, runs: Iterable<{ at: number; count: number; home: readonly number[] }>, cut: Cut, level: number) {
  const out: number[] = [];
  const riser = (axis: number, negative: boolean, plane: number, lowU: number, lowV: number, sizeU: number, sizeV: number, material: number) => {
    out.length += 4;
    packFace(out, out.length - 4, axis, negative, plane, lowU, lowV, sizeU, sizeV, material, 0, level, 0);
  };
  for (const { at, count, home: [x, , z] } of runs) {
    const besideX = (x < cut.x0 && x + Side >= cut.x0) || (x <= cut.x1 && x + Side > cut.x1);
    const besideZ = (z < cut.z0 && z + Side >= cut.z0) || (z <= cut.z1 && z + Side > cut.z1);
    if (!(besideX && z < cut.z1 && z + Side > cut.z0) && !(besideZ && x < cut.x1 && x + Side > cut.x0)) continue;
    for (let face = at * 4; face < (at + count) * 4; face += 4) {
      // Tops only: axis y, looking up.
      if ((packed[face + 3] & 7) !== 1) continue;
      const top = short(packed[face]);
      const z0 = short(packed[face] >>> 16);
      const x0 = short(packed[face + 1]);
      const z1 = z0 + (packed[face + 1] >>> 16);
      const x1 = x0 + (packed[face + 2] & 0xffff);
      const material = packed[face + 2] >>> 16 & 0xff;
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
