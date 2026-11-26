import { BufferAttribute } from "three";

export interface PatchGrid {
  // Triangles of the (cells + 1)² height grid followed by the skirt walls.
  index: BufferAttribute;
  // Indices into `index` covered by the height grid alone.
  surfaceCount: number;
  // Grid vertex each skirt vertex hangs below. Skirt vertices follow the grid vertices in every attribute.
  hem: Uint32Array;
}

const grids = new Map<number, PatchGrid>();

// Grid vertices are row-major with Z as the row: index = z * (cells + 1) + x. Cells split along the diagonal from
// their minimum to their maximum corner, the one the engine's parent heights assume. Faces wind counter-clockwise
// seen from above (+Y) and from outside the patch for the skirt walls.
export function patchGrid(cells: number): PatchGrid {
  const known = grids.get(cells);
  if (known) return known;
  const side = cells + 1;
  const last = side * cells;
  const hem = new Uint32Array(side * 4);
  for (let i = 0; i < side; i++) {
    hem[i] = i;
    hem[side + i] = last + i;
    hem[side * 2 + i] = i * side;
    hem[side * 3 + i] = i * side + cells;
  }
  const vertices = side * side + hem.length;
  const indices = vertices <= 65536 ? new Uint16Array(cells * cells * 6 + cells * 24) : new Uint32Array(cells * cells * 6 + cells * 24);
  let out = 0;
  for (let z = 0; z < cells; z++) {
    for (let x = 0; x < cells; x++) {
      const a = z * side + x;
      const c = a + side;
      indices[out++] = a;
      indices[out++] = c;
      indices[out++] = c + 1;
      indices[out++] = a;
      indices[out++] = c + 1;
      indices[out++] = a + 1;
    }
  }
  const surfaceCount = out;
  // North and east walls keep the edge direction, south and west walls reverse it, so each wall faces outward.
  for (let edge = 0; edge < 4; edge++) {
    const forward = edge === 0 || edge === 3;
    for (let i = 0; i < cells; i++) {
      const top = hem[edge * side + i];
      const next = hem[edge * side + i + 1];
      const low = side * side + edge * side + i;
      const first = forward ? next : low;
      const second = forward ? low : next;
      indices[out++] = top;
      indices[out++] = first;
      indices[out++] = second;
      indices[out++] = next;
      indices[out++] = forward ? low + 1 : low;
      indices[out++] = forward ? low : low + 1;
    }
  }
  const grid = { index: new BufferAttribute(indices, 1), surfaceCount, hem };
  grids.set(cells, grid);
  return grid;
}
