import { BufferAttribute, BufferGeometry, Sphere, Vector3 } from "three";
import type { FarPatch } from "../../../contract/GameBridge";
import { patchGrid } from "./patchGrid";

// Cells wider than this are too coarse to carry a waterline's wet band.
const ShoreCell = 4;

// Positions are metres from the patch origin. `parent` is the height the coarser level has at the same place,
// which the surface shader blends towards before the patch gives way to its parent. `across` runs 0…1 over the
// patch for its shading raster, and `shore` is how wet the ground is from the standing water next to it.
export function patchGeometry(patch: FarPatch) {
  const { cells, heights, morph, normals, skirt, water } = patch;
  const grid = patchGrid(cells);
  const side = cells + 1;
  const count = side * side;
  const total = count + grid.hem.length;
  const spacing = patch.size / cells;
  const positions = new Float32Array(total * 3);
  const parents = new Float32Array(total);
  const directions = new Int8Array(total * 3);
  const across = new Float32Array(total * 2);
  const shore = new Uint8Array(total);
  let low = Infinity;
  let high = -Infinity;

  for (let i = 0; i < count; i++) {
    const height = heights[i];
    positions[i * 3] = (i % side) * spacing;
    positions[i * 3 + 1] = height;
    positions[i * 3 + 2] = Math.floor(i / side) * spacing;
    low = Math.min(low, height, morph[i]);
    high = Math.max(high, height, morph[i]);
    across[i * 2] = (i % side) / cells;
    across[i * 2 + 1] = Math.floor(i / side) / cells;
    // Soaked at and below the water's edge, dry half a metre above it.
    if (spacing <= ShoreCell && !Number.isNaN(water[i])) shore[i] = Math.round(255 * Math.max(0, Math.min(1, 1 - (height - water[i]) / 0.5)));
  }
  parents.set(morph);
  directions.set(normals);
  for (let i = 0; i < grid.hem.length; i++) {
    const from = grid.hem[i];
    const to = count + i;
    positions[to * 3] = positions[from * 3];
    positions[to * 3 + 1] = heights[from] - skirt;
    positions[to * 3 + 2] = positions[from * 3 + 2];
    parents[to] = morph[from] - skirt;
    shore[to] = shore[from];
    across[to * 2] = across[from * 2];
    across[to * 2 + 1] = across[from * 2 + 1];
    for (let axis = 0; axis < 3; axis++) directions[to * 3 + axis] = normals[from * 3 + axis];
  }

  const geometry = new BufferGeometry();
  geometry.setAttribute("position", new BufferAttribute(positions, 3));
  geometry.setAttribute("parent", new BufferAttribute(parents, 1));
  geometry.setAttribute("normal", new BufferAttribute(directions, 3, true));
  geometry.setAttribute("across", new BufferAttribute(across, 2));
  geometry.setAttribute("shore", new BufferAttribute(shore, 1, true));
  geometry.setIndex(grid.index);
  low -= skirt;
  const half = patch.size / 2;
  geometry.boundingSphere = new Sphere(new Vector3(half, (low + high) / 2, half), Math.hypot(half, half, (high - low) / 2));
  return geometry;
}

// The index is shared between patches of one grid size and must outlive each of them.
export function disposePatchGeometry(geometry: BufferGeometry) {
  geometry.setIndex(null);
  geometry.dispose();
}
