import { BufferAttribute, BufferGeometry, Sphere, Vector3 } from "three";
import type { FarPatch } from "../../../contract/GameBridge";
import { patchGrid } from "../../terrain/fn/patchGrid";

// Water surface of one far patch on the patch's own grid, none when the patch is dry. The engine gives dry
// vertices next to water that water's level, so a cell is drawn when all four corners carry one. Terrain then cuts
// the shoreline by depth and the `depth` attribute (water above ground, negative under the shore) fades the edge.
// Standing water is level: a cell whose corners disagree by more than a lake's own slope could is where two
// waters meet or where the level is not a surface at all, and is left open; `open` counts those.
export function naturalWaterGeometry(patch: FarPatch) {
  const { cells, heights, water } = patch;
  const grid = patchGrid(cells);
  const side = cells + 1;
  const count = side * side;
  const spacing = patch.size / cells;
  const positions = new Float32Array(count * 3);
  const depths = new Float32Array(count);
  let levelled = 0;
  let low = Infinity;
  let high = -Infinity;

  for (let i = 0; i < count; i++) {
    const level = water[i];
    const known = !Number.isNaN(level);
    if (known) {
      levelled++;
      low = Math.min(low, level);
      high = Math.max(high, level);
    }
    positions[i * 3] = (i % side) * spacing;
    positions[i * 3 + 1] = known ? level : 0;
    positions[i * 3 + 2] = Math.floor(i / side) * spacing;
    depths[i] = known ? level - heights[i] : 0;
  }
  if (!levelled) return { geometry: null, open: 0 };

  const geometry = new BufferGeometry();
  geometry.setAttribute("position", new BufferAttribute(positions, 3));
  geometry.setAttribute("depth", new BufferAttribute(depths, 1));
  const tilt = Math.max(0.3, spacing * 0.004);
  let open = 0;
  // Whether a cell is drawn. A dry corner makes the spread NaN, which is neither level nor counted.
  const level = (cell: number) => {
    const corner = Math.floor(cell / cells) * side + cell % cells;
    const [a, b, c, d] = [water[corner], water[corner + 1], water[corner + side], water[corner + side + 1]];
    const spread = Math.max(a, b, c, d) - Math.min(a, b, c, d);
    if (spread > tilt) open++;
    return spread <= tilt;
  };
  if (levelled === count && high - low <= tilt) {
    geometry.setIndex(grid.index);
    geometry.setDrawRange(0, grid.surfaceCount);
  } else {
    const source = grid.index.array;
    const indices = source instanceof Uint16Array ? new Uint16Array(grid.surfaceCount) : new Uint32Array(grid.surfaceCount);
    let out = 0;
    for (let cell = 0; cell < cells * cells; cell++) {
      if (!level(cell)) continue;
      for (let i = 0; i < 6; i++) indices[out++] = source[cell * 6 + i];
    }
    if (!out) return { geometry: null, open };
    geometry.setIndex(new BufferAttribute(indices.slice(0, out), 1));
    geometry.userData.ownsIndex = true;
  }
  const half = patch.size / 2;
  geometry.boundingSphere = new Sphere(new Vector3(half, (low + high) / 2, half), Math.hypot(half, half, (high - low) / 2));
  return { geometry, open };
}

// A fully wet patch borrows the shared grid index, which must outlive it.
export function disposeNaturalWaterGeometry(geometry: BufferGeometry) {
  if (!geometry.userData.ownsIndex) geometry.setIndex(null);
  geometry.dispose();
}
