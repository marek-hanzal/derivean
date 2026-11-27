import type { Node } from "three/webgpu";
import { attribute, float, modelWorldMatrix, positionGeometry, select, uint, varying, vec2, vec3 } from "three/tsl";
import { Face, type FaceField, Upright, fieldMask } from "../constant/layout";

// What the shader stages read of a packed face (packFaces). `corner` is the vertex in cubes from the batch's
// anchor, for the material's position node. The rest reaches the fragment stage: the face's normal in render
// space, where the fragment lies across the quad (0…1 on both axes, lowU and lowV at zero), and the fields of
// the face, uninterpolated: `field` reads one of them as an unsigned number.
export function faceNodes() {
  const face = attribute("face", "uvec4");
  const words = [face.x, face.y, face.z, face.w];
  const raw = (field: FaceField) => words[field.word].shiftRight(uint(field.shift)).bitAnd(uint(fieldMask(field)));
  const signed = (value: Node<"float">) => select(value.greaterThanEqual(32768), value.sub(65536), value);
  const lowU = signed(float(raw(Face.lowU)));
  const lowV = signed(float(raw(Face.lowV)));
  const sizeU = float(raw(Face.sizeU));
  const sizeV = float(raw(Face.sizeV));
  const axis = raw(Face.axis);
  const negative = raw(Face.negative).equal(uint(1));
  const rise = float(raw(Face.rise)).div(256);
  // A face that looks down its axis runs its U the other way, which turns its winding to face that way.
  const across = vec2(select(negative, positionGeometry.x.oneMinus(), positionGeometry.x), positionGeometry.y);
  const u = lowU.add(across.x.mul(sizeU));
  const v = lowV.add(across.y.mul(sizeV));
  const [midU, midV] = [lowU.add(sizeU.mul(0.5)), lowV.add(sizeV.mul(0.5))];
  const onX = axis.equal(uint(0));
  const onY = axis.equal(uint(Upright));
  const facing = select(negative, float(-1), float(1));
  // A cap stands a sixty-fourth of a cube behind its plane.
  const plane = signed(float(raw(Face.plane))).sub(facing.mul(float(raw(Face.cap))).div(64));
  const normal = select(onX, vec3(facing, 0, 0), select(onY, vec3(0, facing, 0), vec3(0, 0, facing)));
  const carried = varying(face.zw, "vFaceWords");
  const late = [undefined, undefined, carried.x, carried.y];
  const place = (atU: Node<"float">, atV: Node<"float">) =>
    select(onX, vec3(plane, atU, atV), select(onY, vec3(atV, plane, atU), vec3(atU, atV, plane))).add(vec3(0, rise, 0));
  return {
    corner: place(u, v),
    // The middle of the face, the same at all four corners. For the vertex stage.
    middle: place(midU, midV),
    normal: varying(normal.transformDirection(modelWorldMatrix), "vFaceNormal"),
    across: varying(across, "vFaceAcross"),
    // A top: the face of a cube that looks up. For the vertex stage.
    top: onY.and(negative.not()),
    // A face and not a hole: a batch's holes are zeroed words, whose quads have no area. For the vertex stage.
    held: sizeU.greaterThan(0),
    // For the fragment stage; only the fields of the last two words are carried there.
    field(field: FaceField): Node<"uint"> {
      const carriedWord = late[field.word];
      if (!carriedWord) throw new Error("A face field of the first two words does not reach the fragment stage");
      return carriedWord.shiftRight(uint(field.shift)).bitAnd(uint(fieldMask(field)));
    },
  };
}

export type FaceNodes = ReturnType<typeof faceNodes>;
