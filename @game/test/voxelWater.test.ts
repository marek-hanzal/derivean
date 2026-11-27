import assert from "node:assert/strict";
import test from "node:test";
import { Group, type InstancedBufferAttribute, type Mesh, MeshBasicNodeMaterial } from "three/webgpu";
import type { ChunkMesh, Frame, Point, WorldInfo } from "../contract/GameBridge";
import { createVoxelWater } from "../src/water/service/VoxelWater";

// Flowing water sends a chunk's mesh tick after tick. The new mesh goes into the chunk's batch in place: the same
// mesh, geometry and face buffer, drawing the new faces only. An unloaded chunk that comes back gets a new batch.
const words = 9;
const world = { voxelSize: 0.125, waterUnitsPerVoxel: 256, chunkMeters: 4, faceWords: words } as unknown as WorldInfo;
const faces = (count: number, plane: number) => Int16Array.from({ length: count * words }, (_, at) => [1, 1, plane, at / words % 32, 0, 1, 1, 0, 0][at % words]);
const mesh = (key: Point, origin: Point, list: Int16Array): ChunkMesh => ({ key, origin, faces: list, quads: null });
const frame = (waterMeshes: ChunkMesh[]) => ({ unloaded: [], waterMeshes }) as unknown as Frame;

test("a chunk's new water mesh is written into its batch in place", () => {
  const content = new Group();
  const water = createVoxelWater(content, world, new MeshBasicNodeMaterial());
  water.apply(frame([mesh([0, 0, 0], [0, 0, 0], faces(12, 3))]));
  const first = content.children[0] as Mesh;
  const buffer = (first.geometry.getAttribute("face") as InstancedBufferAttribute).array;
  assert.equal(first.renderOrder, 2);
  for (const count of [5, 20, 1]) {
    water.apply(frame([mesh([0, 0, 0], [0, 0, 0], faces(count, 4))]));
    assert.equal(content.children.length, 1);
    const now = content.children[0] as Mesh;
    assert.equal(now, first, "the same mesh");
    assert.equal((now.geometry.getAttribute("face") as InstancedBufferAttribute).array, buffer, "the same face buffer");
    assert.equal((now.geometry as unknown as { instanceCount: number }).instanceCount, count, "drawing the new faces only");
  }
  water.apply({ unloaded: [[0, 0, 0]], waterMeshes: [] } as unknown as Frame);
  assert.equal(content.children.length, 0, "an unloaded chunk is gone");
  water.apply(frame([mesh([0, 0, 0], [0, 0, 0], faces(3, 1))]));
  assert.equal(content.children.length, 1);
  assert.notEqual(content.children[0], first, "and comes back as a new batch");
  assert.equal(content.children[0].renderOrder, 2);
  water.apply(frame([mesh([0, 0, 0], [0, 0, 0], new Int16Array(0))]));
  assert.equal(content.children.length, 0, "an empty mesh removes the chunk");
});
