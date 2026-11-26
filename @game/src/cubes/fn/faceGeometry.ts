import { BufferAttribute, InstancedBufferAttribute, InstancedBufferGeometry } from "three/webgpu";

// Faces as one instanced quad: the four corners are the geometry, and every face an instance of four packed words
// (packFaces) that the vertex stage expands (faceNodes). `count` faces are drawn; the array may be longer and be
// filled later.
export function faceGeometry(packed: Uint32Array, count: number) {
  const geometry = new InstancedBufferGeometry();
  geometry.setAttribute("position", new BufferAttribute(new Float32Array([0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0]), 3));
  geometry.setIndex(new BufferAttribute(new Uint16Array([0, 1, 2, 2, 1, 3]), 1));
  geometry.setAttribute("face", new InstancedBufferAttribute(packed, 4));
  geometry.instanceCount = count;
  return geometry;
}
