import type { Quads } from "../../../contract/GameBridge";
import { Face, type FaceField, FaceWords, fieldMask } from "../constant/layout";

// What the cube pipeline reads on the GPU (faceNodes): four 32-bit words a face, laid out as constant/layout says.
// Plane, lowU and lowV are signed sixteen-bit counts of cubes from the anchor of the batch the face is drawn in,
// so the place of its chunk is already in them. `rise` lifts the face by 256ths of a cube: a sheet of water lies
// at its level, not on the lattice. A cap (capFaces) stands a sliver behind its plane, clear of a riser that lies
// in it.
const put = (field: FaceField, value: number) => (value & fieldMask(field)) << field.shift;

export const packFace = (out: Uint32Array | number[], at: number, axis: number, negative: boolean, plane: number, lowU: number, lowV: number,
  sizeU: number, sizeV: number, material: number, occlusion: number, level: number, rise: number, cap = false) => {
  out[at] = (put(Face.plane, plane) | put(Face.lowU, lowU)) >>> 0;
  out[at + 1] = (put(Face.lowV, lowV) | put(Face.sizeU, sizeU)) >>> 0;
  out[at + 2] = (put(Face.sizeV, sizeV) | put(Face.material, material) | put(Face.occlusion, occlusion)) >>> 0;
  out[at + 3] = (put(Face.axis, axis) | put(Face.negative, Number(negative)) | put(Face.level, level) | put(Face.rise, rise)
    | put(Face.cap, Number(cap))) >>> 0;
};

const get = (packed: Uint32Array, at: number, field: FaceField) => packed[at + field.word] >>> field.shift & fieldMask(field);
// A sixteen-bit field read as a signed count.
const signed = (value: number) => value << 16 >> 16;

// A packed face read back, for the code that builds faces from faces (capFaces).
export function readFace(packed: Uint32Array, at: number) {
  return {
    axis: get(packed, at, Face.axis),
    negative: get(packed, at, Face.negative) === 1,
    plane: signed(get(packed, at, Face.plane)),
    lowU: signed(get(packed, at, Face.lowU)),
    lowV: signed(get(packed, at, Face.lowV)),
    sizeU: get(packed, at, Face.sizeU),
    sizeV: get(packed, at, Face.sizeV),
    material: get(packed, at, Face.material),
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
    out[at + Face.plane.word] = (put(Face.plane, plane + by[axis]) | put(Face.lowU, lowU + by[(axis + 1) % 3])) >>> 0;
    out[at + Face.lowV.word] = (put(Face.lowV, lowV + by[(axis + 2) % 3]) | put(Face.sizeU, get(out, at, Face.sizeU))) >>> 0;
  }
}
