import type { Node } from "three/webgpu";
import { attribute, float, modelWorldMatrix, positionGeometry, select, uint, varying, vec2, vec3 } from "three/tsl";
import { Face, Upright } from "../constant/layout";

type Field = { word: number; shift: number };

// What the shader stages read of a packed face (packFaces). `corner` is the vertex in cubes from the batch's
// anchor, for the material's position node. The rest reaches the fragment stage: the face's normal in render
// space, where the fragment lies across the quad (0…1 on both axes, lowU and lowV at zero), and the fields of
// the face, uninterpolated: `field` reads one of them as an unsigned number of `bits` bits.
export function faceNodes() {
  const face = attribute("face", "uvec4");
  const words = [face.x, face.y, face.z, face.w];
  const raw = ({ word, shift }: Field, bits: number) => words[word].shiftRight(uint(shift)).bitAnd(uint(2 ** bits - 1));
  const signed = (value: Node<"float">) => select(value.greaterThanEqual(32768), value.sub(65536), value);
  const lowU = signed(float(raw(Face.lowU, 16)));
  const lowV = signed(float(raw(Face.lowV, 16)));
  const sizeU = float(raw(Face.sizeU, 16));
  const sizeV = float(raw(Face.sizeV, 16));
  const axis = raw(Face.axis, 2);
  const negative = raw(Face.negative, 1).equal(uint(1));
  const rise = float(raw(Face.rise, 8)).div(256);
  // A face that looks down its axis runs its U the other way, which turns its winding to face that way.
  const across = vec2(select(negative, positionGeometry.x.oneMinus(), positionGeometry.x), positionGeometry.y);
  const u = lowU.add(across.x.mul(sizeU));
  const v = lowV.add(across.y.mul(sizeV));
  const onX = axis.equal(uint(0));
  const onY = axis.equal(uint(Upright));
  const facing = select(negative, float(-1), float(1));
  // A cap stands a sixty-fourth of a cube behind its plane.
  const plane = signed(float(raw(Face.plane, 16))).sub(facing.mul(float(raw(Face.cap, 1))).div(64));
  const normal = select(onX, vec3(facing, 0, 0), select(onY, vec3(0, facing, 0), vec3(0, 0, facing)));
  const carried = varying(face.zw, "vFaceWords");
  const late = [undefined, undefined, carried.x, carried.y];
  return {
    corner: select(onX, vec3(plane, u, v), select(onY, vec3(v, plane, u), vec3(u, v, plane))).add(vec3(0, rise, 0)),
    normal: varying(normal.transformDirection(modelWorldMatrix), "vFaceNormal"),
    across: varying(across, "vFaceAcross"),
    // A top: the face of a cube that looks up. For the vertex stage.
    top: onY.and(negative.not()),
    // For the fragment stage; only the fields of the last two words are carried there.
    field({ word, shift }: Field, bits: number): Node<"uint"> {
      const carriedWord = late[word];
      if (!carriedWord) throw new Error("A face field of the first two words does not reach the fragment stage");
      return carriedWord.shiftRight(uint(shift)).bitAnd(uint(2 ** bits - 1));
    },
  };
}

export type FaceNodes = ReturnType<typeof faceNodes>;
