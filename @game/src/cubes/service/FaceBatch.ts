import { type Group, type InstancedBufferAttribute, type InstancedBufferGeometry, type Material, Mesh, type Vector3 } from "three/webgpu";
import type { Point, Quads } from "../../../contract/GameBridge";
import { FaceWords } from "../constant/layout";
import { type Cut, capFaces } from "../fn/capFaces";
import { faceGeometry } from "../fn/faceGeometry";
import { packFaces, packQuads, shiftFaces } from "../fn/packFaces";
import type { Casting } from "../type/Casting";

// A list's instances; for a chunk also its place in cubes from the anchor.
interface Run { at: number; count: number; home?: number[] }

const Caps = "caps";

// Fewer holes than this are not worth closing.
const Tidy = 4096;
// Cubes, and at least this many metres, the render origin may lie from the batch's anchor before the anchor
// follows it: a float32 matrix that far out still places a vertex within a few thousandths of a cube, and a
// face's sixteen bits still reach its chunk.
const AnchorReach = 8192;
const AnchorMetres = 1024;

// Every face list of one cube size as a single draw. Lists come and go by key; each keeps a run of the batch's
// instances, and its faces are counted from the batch's anchor, so the one mesh's matrix (the anchor in world
// doubles, the cube edge as scale) places them all. A list that leaves has its run zeroed, which draws nothing,
// and the runs are closed up once half of the batch is such holes. `side` is cubes along the edge of a chunk:
// anchors lie on the chunks' lattice. A batch that casts shadows has a second mesh over the same faces among the
// casters.
export function createFaceBatch(content: Group, material: Material, level: number, edge: number, side: number, casting?: Casting) {
  const runs = new Map<number | string, Run>();
  let packed = new Uint32Array(0);
  let used = 0;
  let holes = 0;
  let anchor: Point = [0, 0, 0];
  let cut: Cut | undefined;
  let capped = false;
  let revision = 0;
  let mesh: Mesh<InstancedBufferGeometry> | undefined;
  let caster: Mesh<InstancedBufferGeometry> | undefined;

  function place(body: Mesh) {
    body.frustumCulled = false;
    body.matrixAutoUpdate = false;
    body.scale.setScalar(edge);
    body.position.set(...anchor);
    body.updateMatrix();
  }

  // Replaces the geometry: the buffer has grown or everything in it has moved.
  function rebuild() {
    const geometry = faceGeometry(packed, used);
    revision++;
    if (mesh) {
      mesh.geometry.dispose();
      mesh.geometry = geometry;
      if (caster) caster.geometry = geometry;
      return;
    }
    mesh = new Mesh(geometry, material);
    place(mesh);
    content.add(mesh);
    if (!casting) return;
    caster = new Mesh(geometry, casting.material);
    caster.layers.set(casting.layer);
    place(caster);
    casting.group.add(caster);
  }

  function touch(at: number, count: number) {
    if (!mesh) return;
    const faces = mesh.geometry.getAttribute("face") as InstancedBufferAttribute;
    faces.addUpdateRange(at * FaceWords, count * FaceWords);
    faces.needsUpdate = true;
    mesh.geometry.instanceCount = used;
    revision++;
  }

  // Closes the holes up, in the order the runs lie in.
  function tidy() {
    let next = 0;
    for (const run of [...runs.values()].sort((a, b) => a.at - b.at)) {
      packed.copyWithin(next * FaceWords, run.at * FaceWords, (run.at + run.count) * FaceWords);
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
    if (run.home) capped = false;
    packed.fill(0, run.at * FaceWords, (run.at + run.count) * FaceWords);
    touch(run.at, run.count);
    holes += run.count;
    if (holes > Tidy && holes * 2 > used) tidy();
  }

  // Makes room for `count` more faces and gives them a run.
  function claim(key: number | string, count: number) {
    if ((used + count) * FaceWords > packed.length) {
      const wider = new Uint32Array(Math.max(1024, 2 ** Math.ceil(Math.log2(used + count))) * FaceWords);
      wider.set(packed.subarray(0, used * FaceWords));
      packed = wider;
      if (mesh) rebuild();
    }
    const run: Run = { at: used, count };
    runs.set(key, run);
    used += count;
    return run;
  }

  function* chunks() {
    for (const run of runs.values()) if (run.home) yield run as Required<Run>;
  }

  function show(run: Run) {
    if (mesh) touch(run.at, run.count);
    else rebuild();
  }

  function moved() {
    for (const body of [mesh, caster]) {
      body?.position.set(...anchor);
      body?.updateMatrix();
    }
    revision++;
  }

  return {
    // Chunks held.
    get lists() { return runs.size - Number(runs.has(Caps)); },
    // Grows whenever what the batch draws has changed.
    get revision() { return revision; },
    remove,
    // A chunk's quads; `origin` is the world position they count from, on the chunks' lattice except in height
    // for a sheet of water, whose odd part of a cube the faces carry as their rise.
    add(key: number | string, quads: Quads, origin: Point) {
      remove(key);
      if (!quads.length) return;
      if (!mesh) anchor = [origin[0], Math.floor(origin[1] / edge) * edge, origin[2]];
      const height = (origin[1] - anchor[1]) / edge;
      const run = claim(key, quads.length / 2);
      run.home = [Math.round((origin[0] - anchor[0]) / edge), Math.floor(height), Math.round((origin[2] - anchor[2]) / edge)];
      packQuads(quads, level, run.home, Math.floor((height - Math.floor(height)) * 256), packed, run.at);
      capped = false;
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
      const lattice = edge * side;
      const next: Point = [Math.round(origin.x / lattice) * lattice, Math.round(origin.y / lattice) * lattice, Math.round(origin.z / lattice) * lattice];
      const by = anchor.map((value, axis) => Math.round((value - next[axis]) / edge));
      shiftFaces(packed, 0, used, by);
      for (const run of runs.values()) run.home = run.home?.map((value, axis) => value + by[axis]);
      capped = false;
      anchor = next;
      moved();
      tidy();
    },
    // Closes the ground where a square of it is left to a finer level (capFaces): `low` and `high` are the
    // square's corners in world metres, on this batch's lattice. Without a square nothing is closed.
    cap(low?: readonly number[], high?: readonly number[]) {
      const next = low && high && mesh ? {
        x0: Math.round((low[0] - anchor[0]) / edge), x1: Math.round((high[0] - anchor[0]) / edge),
        z0: Math.round((low[2] - anchor[2]) / edge), z1: Math.round((high[2] - anchor[2]) / edge),
      } : undefined;
      if (capped && next?.x0 === cut?.x0 && next?.x1 === cut?.x1 && next?.z0 === cut?.z0 && next?.z1 === cut?.z1) return;
      cut = next;
      capped = true;
      const faces = cut && cut.x1 > cut.x0 && cut.z1 > cut.z0 ? capFaces(packed, chunks(), cut, level, side) : [];
      // The caps change with every step of the finer square, so their run is kept and written over; it is only
      // moved when it has become too short.
      let run = runs.get(Caps);
      if (!run && !faces.length) return;
      if (!run || run.count * FaceWords < faces.length) {
        remove(Caps);
        run = claim(Caps, Math.max(256, 2 ** Math.ceil(Math.log2(faces.length / FaceWords))));
      }
      packed.fill(0, run.at * FaceWords, (run.at + run.count) * FaceWords);
      packed.set(faces, run.at * FaceWords);
      show(run);
    },
    // The mesh, for a batch whose draw order is set. Undefined while the batch is empty.
    get mesh() { return mesh; },
    // Poses a batch that is a body of its own.
    pose(position: Point, rotation: readonly [number, number, number, number]) {
      for (const body of [mesh, caster]) {
        body?.position.set(...position);
        body?.quaternion.set(...rotation);
        body?.updateMatrix();
      }
      revision++;
    },
    dispose() {
      if (!mesh) return;
      content.remove(mesh);
      if (caster) casting?.group.remove(caster);
      mesh.geometry.dispose();
      mesh = caster = undefined;
      runs.clear();
      used = holes = 0;
    },
  };
}

export type FaceBatch = ReturnType<typeof createFaceBatch>;
