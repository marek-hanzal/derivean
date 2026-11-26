import type { Quads } from "../../../contract/GameBridge";
import { Face, FaceWords } from "../constant/layout";

// What the cube pipeline reads on the GPU (faceNodes): four 32-bit words a face, laid out as constant/layout says.
// Plane, lowU and lowV are signed sixteen-bit counts of cubes from the anchor of the batch the face is drawn in,
// so the place of its chunk is already in them. `rise` lifts the face by 256ths of a cube: a sheet of water lies
// at its level, not on the lattice. A cap (capFaces) stands a sliver behind its plane, clear of a riser that lies
// in it.
export const packFace = (out: Uint32Array | number[], at: number, axis: number, negative: boolean, plane: number, lowU: number, lowV: number,
  sizeU: number, sizeV: number, material: number, occlusion: number, level: number, rise: number, cap = false) => {
  out[at] = (plane & 0xffff) << Face.plane.shift | (lowU & 0xffff) << Face.lowU.shift;
  out[at + 1] = (lowV & 0xffff) << Face.lowV.shift | (sizeU & 0xffff) << Face.sizeU.shift;
  out[at + 2] = (sizeV & 0xffff) << Face.sizeV.shift | (material & 0xff) << Face.material.shift | (occlusion & 0xff) << Face.occlusion.shift;
  out[at + 3] = axis << Face.axis.shift | Number(negative) << Face.negative.shift | level << Face.level.shift | rise << Face.rise.shift
    | Number(cap) << Face.cap.shift;
};

const signed = (word: number) => (word & 0xffff) << 16 >> 16;

// A packed face read back, for the code that builds faces from faces (capFaces).
export function readFace(packed: Uint32Array, at: number) {
  return {
    axis: packed[at + Face.axis.word] >>> Face.axis.shift & 3,
    negative: (packed[at + Face.negative.word] >>> Face.negative.shift & 1) === 1,
    plane: signed(packed[at + Face.plane.word] >>> Face.plane.shift),
    lowU: signed(packed[at + Face.lowU.word] >>> Face.lowU.shift),
    lowV: signed(packed[at + Face.lowV.word] >>> Face.lowV.shift),
    sizeU: packed[at + Face.sizeU.word] >>> Face.sizeU.shift & 0xffff,
    sizeV: packed[at + Face.sizeV.word] >>> Face.sizeV.shift & 0xffff,
    material: packed[at + Face.material.word] >>> Face.material.shift & 0xff,
  };
}

// The engine's quads of a chunk (Quads), `home` cubes from the anchor, into `out` from face `first` on.
export function packQuads(quads: Quads, level: number, home: readonly number[], rise: number, out: Uint32Array, first: number) {
  for (let quad = 0; quad < quads.length / 2; quad++) {
    const shape = quads[quad * 2];
    const look = quads[quad * 2 + 1];
    const axis = shape >>> 28 & 3;
    packFace(out, (first + quad) * FaceWords, axis, (shape >>> 30 & 1) === 0, (shape & 63) + home[axis], (shape >>> 6 & 63) + home[(axis + 1) % 3],
      (shape >>> 12 & 63) + home[(axis + 2) % 3], (shape >>> 18 & 31) + 1, (shape >>> 23 & 31) + 1, look & 255, look >>> 8 & 255, level, rise);
  }
}

// An engine face list of `words` shorts a quad (ChunkMesh.faces), counted from the anchor itself.
export function packFaces(faces: Int16Array, words: number, level: number, out: Uint32Array, first: number) {
  for (let face = 0; face < Math.floor(faces.length / words); face++) {
    const at = face * words;
    packFace(out, (first + face) * FaceWords, faces[at], faces[at + 1] < 0, faces[at + 2], faces[at + 3], faces[at + 4], faces[at + 5], faces[at + 6],
      faces[at + 7], words > 8 ? faces[at + 8] : 0, level, 0);
  }
}

// Moves `count` packed faces from `first` on by whole cubes: the batch's anchor has moved the other way.
export function shiftFaces(out: Uint32Array, first: number, count: number, by: readonly number[]) {
  for (let at = first * FaceWords; at < (first + count) * FaceWords; at += FaceWords) {
    const { axis, plane, lowU, lowV } = readFace(out, at);
    out[at] = ((plane + by[axis]) & 0xffff) << Face.plane.shift | ((lowU + by[(axis + 1) % 3]) & 0xffff) << Face.lowU.shift;
    out[at + 1] = ((lowV + by[(axis + 2) % 3]) & 0xffff) << Face.lowV.shift | (out[at + 1] & 0xffff << Face.sizeU.shift);
  }
}
