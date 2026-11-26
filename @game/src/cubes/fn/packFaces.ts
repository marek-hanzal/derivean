import type { Quads } from "../../../contract/GameBridge";

// What the cube pipeline reads on the GPU (faceNodes): four 32-bit words a face.
//   0  plane | lowU << 16            2  sizeV | material << 16 | occlusion << 24
//   1  lowV | sizeU << 16            3  axis | negative << 2 | level << 3 | rise << 7 | cap << 15
// Plane, lowU and lowV are signed sixteen-bit counts of cubes from the anchor of the batch the face is drawn in,
// so the place of its chunk is already in them. `rise` lifts the face by 256ths of a cube: a sheet of water lies
// at its level, not on the lattice. A cap (capFaces) stands a sliver behind its plane, clear of a riser that lies
// in it.
export const packFace = (out: Uint32Array | number[], at: number, axis: number, negative: boolean, plane: number, lowU: number, lowV: number,
  sizeU: number, sizeV: number, material: number, occlusion: number, level: number, rise: number, cap = false) => {
  out[at] = (plane & 0xffff) | (lowU & 0xffff) << 16;
  out[at + 1] = (lowV & 0xffff) | (sizeU & 0xffff) << 16;
  out[at + 2] = (sizeV & 0xffff) | (material & 0xff) << 16 | (occlusion & 0xff) << 24;
  out[at + 3] = axis | (negative ? 4 : 0) | level << 3 | rise << 7 | (cap ? 1 << 15 : 0);
};

// The engine's quads of a chunk (Quads), `home` cubes from the anchor, into `out` from face `first` on.
export function packQuads(quads: Quads, level: number, home: readonly number[], rise: number, out: Uint32Array, first: number) {
  for (let quad = 0; quad < quads.length / 2; quad++) {
    const shape = quads[quad * 2];
    const look = quads[quad * 2 + 1];
    const axis = shape >>> 28 & 3;
    packFace(out, (first + quad) * 4, axis, (shape >>> 30 & 1) === 0, (shape & 63) + home[axis], (shape >>> 6 & 63) + home[(axis + 1) % 3],
      (shape >>> 12 & 63) + home[(axis + 2) % 3], (shape >>> 18 & 31) + 1, (shape >>> 23 & 31) + 1, look & 255, look >>> 8 & 255, level, rise);
  }
}

// An engine face list of `words` shorts a quad (ChunkMesh.faces), counted from the anchor itself.
export function packFaces(faces: Int16Array, words: number, level: number, out: Uint32Array, first: number) {
  for (let face = 0; face < Math.floor(faces.length / words); face++) {
    const at = face * words;
    packFace(out, (first + face) * 4, faces[at], faces[at + 1] < 0, faces[at + 2], faces[at + 3], faces[at + 4], faces[at + 5], faces[at + 6],
      faces[at + 7], words > 8 ? faces[at + 8] : 0, level, 0);
  }
}

// Moves `count` packed faces from `first` on by whole cubes: the batch's anchor has moved the other way.
export function shiftFaces(out: Uint32Array, first: number, count: number, by: readonly number[]) {
  for (let at = first * 4; at < (first + count) * 4; at += 4) {
    const axis = out[at + 3] & 3;
    const plane = (out[at] & 0xffff) + by[axis];
    const lowU = (out[at] >>> 16) + by[(axis + 1) % 3];
    out[at] = (plane & 0xffff) | (lowU & 0xffff) << 16;
    out[at + 1] = (((out[at + 1] & 0xffff) + by[(axis + 2) % 3]) & 0xffff) | (out[at + 1] & 0xffff0000);
  }
}
