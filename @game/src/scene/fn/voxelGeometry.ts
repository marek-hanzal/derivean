import { BufferAttribute, BufferGeometry, Color } from "three";
import type { WorldInfo } from "../../../contract/GameBridge";

export function voxelGeometry(faces: Int16Array, world: WorldInfo, colors: readonly Color[], unitsPerVoxel = 1) {
  const quads = faces.length / 8;
  const positions = new Float32Array(quads * 12);
  const normals = new Int8Array(quads * 12);
  const palette = new Uint8Array(quads * 12);
  const roughness = new Uint8Array(quads * 4);
  const indices = quads * 4 <= 65536 ? new Uint16Array(quads * 6) : new Uint32Array(quads * 6);
  const tile = world.voxelSize / unitsPerVoxel;

  for (let q = 0; q < quads; q++) {
    const i = q * 8;
    const axis = faces[i];
    const sign = faces[i + 1];
    const u = (axis + 1) % 3;
    const v = (axis + 2) % 3;
    const material = faces[i + 7];
    const color = colors[material];
    const rough = Math.round(world.materials[material].roughness * 255);
    const plane = faces[i + 2] * tile;
    const lowU = faces[i + 3] * tile;
    const lowV = faces[i + 4] * tile;
    const highU = lowU + faces[i + 5] * tile;
    const highV = lowV + faces[i + 6] * tile;

    for (let corner = 0; corner < 4; corner++) {
      const vertex = q * 4 + corner;
      const out = vertex * 3;
      positions[out + axis] = plane;
      positions[out + u] = corner === 1 || corner === 2 ? highU : lowU;
      positions[out + v] = corner >= 2 ? highV : lowV;
      normals[out + axis] = sign * 127;
      palette[out] = Math.round(color.r * 255);
      palette[out + 1] = Math.round(color.g * 255);
      palette[out + 2] = Math.round(color.b * 255);
      roughness[vertex] = rough;
    }

    const base = q * 4;
    const index = q * 6;
    indices[index] = base;
    indices[index + 1] = base + (sign > 0 ? 1 : 2);
    indices[index + 2] = base + (sign > 0 ? 2 : 1);
    indices[index + 3] = base;
    indices[index + 4] = base + (sign > 0 ? 2 : 3);
    indices[index + 5] = base + (sign > 0 ? 3 : 2);
  }
  const geometry = new BufferGeometry();
  geometry.setAttribute("position", new BufferAttribute(positions, 3));
  geometry.setAttribute("normal", new BufferAttribute(normals, 3, true));
  geometry.setAttribute("color", new BufferAttribute(palette, 3, true));
  geometry.setAttribute("surfaceRoughness", new BufferAttribute(roughness, 1, true));
  geometry.setIndex(new BufferAttribute(indices, 1));
  geometry.computeBoundingSphere();
  return geometry;
}
