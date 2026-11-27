import assert from "node:assert/strict";
import test from "node:test";
import { Group, type InstancedBufferAttribute, MeshBasicNodeMaterial, Vector3 } from "three/webgpu";
import { FaceWords } from "../src/cubes/constant/layout";
import { createFaceBatch } from "../src/cubes/service/FaceBatch";

// A batch whose lists come and go reuses its holes and stops drawing the ones at its end; what it draws must be
// the faces of the lists it holds, every one once, as a batch built from those lists alone draws them.
function drawn(batch: ReturnType<typeof createFaceBatch>) {
  const geometry = batch.mesh!.geometry;
  const words = (geometry.getAttribute("face") as InstancedBufferAttribute).array as Uint32Array;
  const faces: string[] = [];
  for (let face = 0; face < geometry.instanceCount; face++) {
    const at = face * FaceWords;
    if (words[at] || words[at + 1] || words[at + 2] || words[at + 3]) faces.push(words.slice(at, at + FaceWords).join(","));
  }
  return { faces: faces.sort(), count: geometry.instanceCount };
}

test("a batch draws the faces of the lists it holds, holes reused and the end trimmed", () => {
  let seed = 7;
  const random = () => ((seed = (seed * 1103515245 + 12345) % 2147483648) / 2147483648);
  const lists = new Map<number, Uint32Array>();
  const make = () => {
    const quads: number[] = [];
    for (let quad = 1 + Math.floor(random() * 300); quad > 0; quad--) {
      quads.push((1 << 30 | 1 << 28 | Math.floor(random() * 31) << 23 | Math.floor(random() * 31) << 18
        | Math.floor(random() * 32) << 12 | Math.floor(random() * 32) << 6 | Math.floor(random() * 64)) >>> 0, Math.floor(random() * 2 ** 32) >>> 0);
    }
    return Uint32Array.from(quads);
  };
  const batch = createFaceBatch(new Group(), new MeshBasicNodeMaterial(), 1, 2, 32);
  for (let step = 0; step < 3000; step++) {
    const key = Math.floor(random() * 120);
    if (lists.has(key) && random() < 0.5) {
      batch.remove(key);
      lists.delete(key);
    } else {
      const quads = make();
      batch.add(key, quads, [0, 0, 0]);
      lists.set(key, quads);
    }
    if (step % 100 || !lists.size) continue;
    const fresh = createFaceBatch(new Group(), new MeshBasicNodeMaterial(), 1, 2, 32);
    for (const [held, quads] of lists) fresh.add(held, quads, [0, 0, 0]);
    const [ours, theirs] = [drawn(batch), drawn(fresh)];
    assert.deepEqual(ours.faces, theirs.faces, `step ${step}`);
    // No hole is drawn at the end: the last instance drawn is a face.
    const geometry = batch.mesh!.geometry;
    const words = (geometry.getAttribute("face") as InstancedBufferAttribute).array as Uint32Array;
    const last = (geometry.instanceCount - 1) * FaceWords;
    assert.ok(words[last] || words[last + 1] || words[last + 2] || words[last + 3], `step ${step}: a hole drawn at the end`);
  }
});

test("a batch closed up in place and moved to a new anchor draws what a batch built afresh does", () => {
  let seed = 11;
  const random = () => ((seed = (seed * 1103515245 + 12345) % 2147483648) / 2147483648);
  const make = () => {
    const quads: number[] = [];
    for (let quad = 200; quad > 0; quad--) {
      quads.push((1 << 30 | 1 << 28 | Math.floor(random() * 31) << 23 | Math.floor(random() * 31) << 18
        | Math.floor(random() * 32) << 12 | Math.floor(random() * 32) << 6 | Math.floor(random() * 64)) >>> 0, Math.floor(random() * 2 ** 32) >>> 0);
    }
    return Uint32Array.from(quads);
  };
  const lists = new Map<number, Uint32Array>();
  const batch = createFaceBatch(new Group(), new MeshBasicNodeMaterial(), 1, 2, 32);
  for (let key = 0; key < 100; key++) {
    lists.set(key, make());
    batch.add(key, lists.get(key)!, [0, 0, 0]);
  }
  // Seven lists of ten go, none of them last: the holes pass half the batch and it is closed up.
  const gone = [...lists.keys()].filter((key) => key % 10 < 7);
  for (const key of gone.sort(() => random() - 0.5)) {
    batch.remove(key);
    lists.delete(key);
  }
  const far = new Vector3(20000, 0, 0);
  const fresh = (moved: boolean) => {
    const built = createFaceBatch(new Group(), new MeshBasicNodeMaterial(), 1, 2, 32);
    for (const [key, quads] of lists) built.add(key, quads, [0, 0, 0]);
    if (moved) built.follow(far);
    return drawn(built);
  };
  // Without closing up, the last list would keep the batch 100 lists long.
  assert.ok(drawn(batch).count < 70 * 200, `closed up, the batch draws ${drawn(batch).count} instances`);
  assert.deepEqual(drawn(batch).faces, fresh(false).faces);
  batch.follow(far);
  assert.deepEqual(drawn(batch).faces, fresh(true).faces);
  for (let key = 100; key < 110; key++) {
    lists.set(key, make());
    batch.add(key, lists.get(key)!, [0, 0, 0]);
  }
  assert.deepEqual(drawn(batch).faces, fresh(true).faces);
});
