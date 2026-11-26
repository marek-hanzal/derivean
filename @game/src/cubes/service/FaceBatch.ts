import { type Group, type InstancedBufferAttribute, type InstancedBufferGeometry, type Material, Mesh, type Vector3 } from "three/webgpu";
import type { Point, Quads } from "../../../contract/GameBridge";
import { faceGeometry } from "../fn/faceGeometry";
import { packFaces, packQuads, shiftFaces } from "../fn/packFaces";

interface Run { at: number; count: number }

// Fewer holes than this are not worth closing.
const Tidy = 4096;
// Cubes, and at least this many metres, the render origin may lie from the batch's anchor before the anchor
// follows it: a float32 matrix that far out still places a vertex within a few thousandths of a cube, and a
// face's sixteen bits still reach its chunk.
const AnchorReach = 8192;
const AnchorMetres = 1024;
// Cubes along the edge of a chunk: anchors lie on the chunks' lattice.
const Side = 32;

// Every face list of one cube size as a single draw. Lists come and go by key; each keeps a run of the batch's
// instances, and its faces are counted from the batch's anchor, so the one mesh's matrix (the anchor in world
// doubles, the cube edge as scale) places them all. A list that leaves has its run zeroed, which draws nothing,
// and the runs are closed up once half of the batch is such holes.
export function createFaceBatch(content: Group, material: Material, level: number, edge: number) {
  const runs = new Map<number | string, Run>();
  let packed = new Uint32Array(0);
  let used = 0;
  let holes = 0;
  let anchor: Point = [0, 0, 0];
  let mesh: Mesh<InstancedBufferGeometry> | undefined;

  // Replaces the geometry: the buffer has grown or everything in it has moved.
  function rebuild() {
    if (mesh) {
      mesh.geometry.dispose();
      mesh.geometry = faceGeometry(packed, used);
      return;
    }
    mesh = new Mesh(faceGeometry(packed, used), material);
    mesh.frustumCulled = false;
    mesh.matrixAutoUpdate = false;
    mesh.scale.setScalar(edge);
    mesh.position.set(...anchor);
    mesh.updateMatrix();
    content.add(mesh);
  }

  function touch(at: number, count: number) {
    if (!mesh) return;
    const faces = mesh.geometry.getAttribute("face") as InstancedBufferAttribute;
    faces.addUpdateRange(at * 4, count * 4);
    faces.needsUpdate = true;
    mesh.geometry.instanceCount = used;
  }

  // Closes the holes up, in the order the runs lie in.
  function tidy() {
    let next = 0;
    for (const run of [...runs.values()].sort((a, b) => a.at - b.at)) {
      packed.copyWithin(next * 4, run.at * 4, (run.at + run.count) * 4);
      run.at = next;
      next += run.count;
    }
    used = next;
    holes = 0;
    rebuild();
  }

  function remove(key: number | string) {
    const run = runs.get(key);
    if (!run) return;
    runs.delete(key);
    packed.fill(0, run.at * 4, (run.at + run.count) * 4);
    touch(run.at, run.count);
    holes += run.count;
    if (holes > Tidy && holes * 2 > used) tidy();
  }

  // Makes room for `count` more faces and gives them a run.
  function claim(key: number | string, count: number) {
    if ((used + count) * 4 > packed.length) {
      const wider = new Uint32Array(Math.max(1024, 2 ** Math.ceil(Math.log2(used + count))) * 4);
      wider.set(packed.subarray(0, used * 4));
      packed = wider;
      mesh?.geometry.dispose();
      if (mesh) mesh.geometry = faceGeometry(packed, used);
    }
    const run = { at: used, count };
    runs.set(key, run);
    used += count;
    return run;
  }

  function show(run: Run) {
    if (mesh) touch(run.at, run.count);
    else rebuild();
  }

  return {
    get lists() { return runs.size; },
    remove,
    // A chunk's quads; `origin` is the world position they count from, on the chunks' lattice except in height
    // for a sheet of water, whose odd part of a cube the faces carry as their rise.
    add(key: number | string, quads: Quads, origin: Point) {
      remove(key);
      if (!quads.length) return;
      if (!mesh) anchor = [origin[0], Math.floor(origin[1] / edge) * edge, origin[2]];
      const height = (origin[1] - anchor[1]) / edge;
      const run = claim(key, quads.length / 2);
      packQuads(quads, level, [Math.round((origin[0] - anchor[0]) / edge), Math.floor(height), Math.round((origin[2] - anchor[2]) / edge)],
        Math.floor((height - Math.floor(height)) * 256), packed, run.at);
      show(run);
    },
    // A face list counted from the batch's own anchor, for a batch that holds nothing else.
    fill(key: number | string, faces: Int16Array, words: number, origin: Point) {
      remove(key);
      const count = Math.floor(faces.length / words);
      if (!count) return;
      if (!mesh) anchor = origin;
      const run = claim(key, count);
      packFaces(faces, words, level, packed, run.at);
      show(run);
    },
    // Keeps the anchor near the render origin, which is in world metres.
    follow(origin: Vector3) {
      if (!mesh || Math.max(Math.abs(origin.x - anchor[0]), Math.abs(origin.y - anchor[1]), Math.abs(origin.z - anchor[2])) < Math.max(AnchorReach * edge, AnchorMetres)) return;
      const lattice = edge * Side;
      const next: Point = [Math.round(origin.x / lattice) * lattice, Math.round(origin.y / lattice) * lattice, Math.round(origin.z / lattice) * lattice];
      shiftFaces(packed, 0, used, anchor.map((value, axis) => Math.round((value - next[axis]) / edge)));
      anchor = next;
      mesh.position.set(...anchor);
      mesh.updateMatrix();
      tidy();
    },
    // The mesh, for a batch that is a body of its own to pose. Undefined while the batch is empty.
    get mesh() { return mesh; },
    dispose() {
      if (!mesh) return;
      content.remove(mesh);
      mesh.geometry.dispose();
      mesh = undefined;
      runs.clear();
    },
  };
}

export type FaceBatch = ReturnType<typeof createFaceBatch>;
