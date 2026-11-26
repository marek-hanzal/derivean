import type { Node } from "three/webgpu";
import { attribute, float, modelWorldMatrix, positionGeometry, select, uint, varying, vec2, vec3 } from "three/tsl";

// What the shader stages read of a packed face (packFaces). `corner` is the vertex in cubes from the batch's
// anchor, for the material's position node. The rest reaches the fragment stage: the face's normal in render
// space, where the fragment lies across the quad (0…1 on both axes, lowU and lowV at zero), and the two words
// that hold material, occlusion and level, uninterpolated.
export function faceNodes() {
  const face = attribute("face", "uvec4");
  const short = (word: Node<"uint">, shift: number) => float(word.shiftRight(uint(shift)).bitAnd(uint(0xffff)));
  const signed = (value: Node<"float">) => select(value.greaterThanEqual(32768), value.sub(65536), value);
  const lowU = signed(short(face.x, 16));
  const lowV = signed(short(face.y, 0));
  const sizeU = short(face.y, 16);
  const sizeV = short(face.z, 0);
  const axis = face.w.bitAnd(uint(3));
  const negative = face.w.shiftRight(uint(2)).bitAnd(uint(1)).equal(uint(1));
  const rise = float(face.w.shiftRight(uint(7)).bitAnd(uint(255))).div(256);
  // A face that looks down its axis runs its U the other way, which turns its winding to face that way.
  const across = vec2(select(negative, positionGeometry.x.oneMinus(), positionGeometry.x), positionGeometry.y);
  const u = lowU.add(across.x.mul(sizeU));
  const v = lowV.add(across.y.mul(sizeV));
  const onX = axis.equal(uint(0));
  const onY = axis.equal(uint(1));
  const facing = select(negative, float(-1), float(1));
  // A cap stands a sixty-fourth of a cube behind its plane.
  const plane = signed(short(face.x, 0)).sub(facing.mul(float(face.w.shiftRight(uint(15)).bitAnd(uint(1)))).div(64));
  const normal = select(onX, vec3(facing, 0, 0), select(onY, vec3(0, facing, 0), vec3(0, 0, facing)));
  return {
    corner: select(onX, vec3(plane, u, v), select(onY, vec3(v, plane, u), vec3(u, v, plane))).add(vec3(0, rise, 0)),
    normal: varying(normal.transformDirection(modelWorldMatrix), "vFaceNormal"),
    across: varying(across, "vFaceAcross"),
    words: varying(face.zw, "vFaceWords"),
  };
}

export type FaceNodes = ReturnType<typeof faceNodes>;
