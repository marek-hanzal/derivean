const { before, test } = require("node:test");
const assert = require("node:assert/strict");
const { setTimeout: sleep } = require("node:timers/promises");
const { Session, world } = require("../../.out/addon/derivean.node");

const controls = {
  elapsed: 1 / 60, x: 0, z: 0, active: true, jump: false, sprint: false,
  direction: [0, 0, -1], radius: 4, shape: "sphere", dig: false, place: false,
  fly: false, boost: false, ascend: 0, teleport: null, time: { rate: 1, jump: 0 },
};

let session;
let info;
// What a renderer would hold of the far cubes: chunk id to its square of ground, and the chunks held over each.
const held = new Map();
const places = new Map();
let occluded = 0;

before(() => {
  session = new Session({ seed: 42, age: 2e9 });
  info = { ...world, ...session.info() };
});

// Unpacks Quads as GameBridge.ts lays them out.
function quads(packed, visit) {
  assert.ok(packed instanceof Uint32Array && packed.length % 2 === 0, "quads are two words each");
  for (let at = 0; at < packed.length; at += 2) {
    const first = packed[at], second = packed[at + 1];
    assert.equal(second >>> 16, 0, "spare bits are zero");
    visit({
      plane: first & 63, u: first >>> 6 & 63, v: first >>> 12 & 63, width: (first >>> 18 & 31) + 1, height: (first >>> 23 & 31) + 1,
      axis: first >>> 28 & 3, positive: first >>> 30 === 1, material: second & 255, occlusion: second >>> 8 & 255,
    });
  }
}

function apply(frame) {
  const { cubes } = frame;
  assert.ok(cubes, "every world sends its far terrain as cubes");
  for (const id of cubes.removed) {
    assert.ok(held.has(id), `cube chunk ${id} was removed but never added`);
    const place = held.get(id);
    if (places.get(place) === 1) places.delete(place);
    else places.set(place, places.get(place) - 1);
    held.delete(id);
  }
  // A square of ground is swapped whole: what was held over it is gone before the new chunks arrive.
  const tops = new Map();
  for (const chunk of cubes.added) assert.ok(!places.has(`${chunk.level}/${chunk.origin[0] / (info.cubes.side * chunk.edge)}/${chunk.origin[2] / (info.cubes.side * chunk.edge)}`), "two builds of one square of ground are held at once");
  for (const chunk of cubes.added) {
    const size = info.cubes.side * chunk.edge;
    const place = `${chunk.level}/${chunk.origin[0] / size}/${chunk.origin[2] / size}`;
    assert.ok(!held.has(chunk.id), `cube chunk ${chunk.id} arrived twice`);
    assert.equal(chunk.edge, info.voxelSize * 2 ** chunk.level);
    assert.ok(chunk.level >= info.cubes.finestLevel && chunk.level <= info.cubes.coarsestLevel);
    assert.ok(chunk.origin.every((value) => Number.isInteger(value / size)), "a chunk lies on the lattice of its level");
    assert.ok(chunk.dataLevel >= 0 && chunk.dataLevel <= chunk.wantedLevel && chunk.wantedLevel <= 4);
    assert.ok(chunk.quads.length > 0, "a chunk without a face is not sent");
    quads(chunk.quads, ({ plane, u, v, width, height, axis, material }) => {
      assert.ok(axis < 3 && plane <= info.cubes.side && u + width <= info.cubes.side && v + height <= info.cubes.side, "a quad keeps to its chunk");
      assert.ok(material > 0 && material < info.materials.length, "a cube face names a material");
      tops.set(place, (tops.get(place) ?? 0) + (axis === 1 ? width * height : 0));
    });
    for (const sheet of chunk.water) {
      assert.ok(Number.isFinite(sheet.level));
      quads(sheet.quads, ({ axis, positive, plane }) => assert.ok(axis === 1 && positive && plane === 0, "water is sent as top faces"));
    }
    held.set(chunk.id, place);
    places.set(place, (places.get(place) ?? 0) + 1);
  }
  for (const [place, area] of tops) assert.equal(area, info.cubes.side ** 2, `every column over ${place} has one top`);
  assert.equal(cubes.data.reduce((sum, count) => sum + count, 0), held.size);
  assert.equal(cubes.levels.length, info.cubes.coarsestLevel - info.cubes.finestLevel + 1);
  let inner = 0;
  for (const level of cubes.levels) {
    assert.ok(level.settled <= level.shown && level.shown <= level.wanted);
    assert.equal(typeof frame.ready.far, "boolean");
    if (frame.ready.far) assert.ok(frame.pending.patches === 0 && frame.cubes.levels.every((each) => each.settled === each.wanted));
    if (!level.box) continue;
    // A box ends a band short of the nearest square not held, so that square itself may lie exactly at `reach`.
    const side = level.box.max[0] - frame.eye[0], size = info.cubes.side * level.edge, reach = side + info.cubes.band * level.edge - 1e-6;
    assert.ok(side >= inner - 1e-6, "boxes are nested from the finest level outwards");
    assert.ok(Math.abs(frame.eye[2] - level.box.min[2] - side) < 1e-6, "a box is centred on the eye");
    inner = side;
    for (let z = Math.floor((frame.eye[2] - reach) / size); z <= Math.floor((frame.eye[2] + reach) / size); z++)
      for (let x = Math.floor((frame.eye[0] - reach) / size); x <= Math.floor((frame.eye[0] + reach) / size); x++)
        assert.ok(places.has(`${level.level}/${x}/${z}`), `level ${level.level} may be drawn where no chunk is held`);
  }
  for (const chunk of frame.chunks) {
    assert.equal(chunk.faces.length, 0, "the ground of a world of cubes comes as quads");
    quads(chunk.quads, ({ occlusion }) => { occluded += occlusion !== 0; });
  }
  for (const mesh of frame.waterMeshes) assert.ok(mesh.quads === null && mesh.faces.length % info.faceWords === 0);
  return frame;
}

async function until(done, seconds, command = controls) {
  for (const start = Date.now(); Date.now() - start < seconds * 1000; await sleep(4)) {
    const frame = apply(session.advance(command));
    if (done(frame)) return frame;
  }
  assert.fail("the world did not get there in time");
}

test("a world of cubes describes its levels", () => {
  assert.equal(info.faceWords, 9);
  assert.ok(info.cubes.reach === 128 && info.cubes.side === 32 && info.cubes.band > 0 && info.cubes.viewDistance >= 250e3);
  assert.equal(info.voxelSize * 2 ** info.cubes.coarsestLevel, 1024);
});

test("the levels of cubes fill from the coarsest and stay whole inside their boxes", { timeout: 900_000 }, async () => {
  const far = (frame) => frame.cubes.levels.at(-1);
  await until((frame) => far(frame).box !== null, 30);
  const whole = (frame) => frame.cubes.levels.every((level) => level.box && level.shown === level.wanted);
  const frame = await until(whole, 300);
  assert.ok(far(frame).box.max[0] - frame.eye[0] >= info.cubes.viewDistance - 1e-6, "the coarsest level reaches the view distance");
  assert.ok(Math.abs(frame.cubes.levels[0].box.max[0] - frame.eye[0] - info.cubes.reach * frame.cubes.levels[0].edge) < 1e-6);
  assert.ok(places.size > 1000 && places.size < 2500 && held.size >= places.size, `${held.size} cube chunks held over ${places.size} squares`);
});

test("voxel chunks carry the occlusion word in a world of cubes", { timeout: 900_000 }, async () => {
  await until((frame) => frame.streamingReady && frame.voxelBox && frame.pending.chunks === 0, 880);
  assert.ok(occluded > 100, "voxel faces next to steps and walls must be shaded at their corners");
});

test("a fast, high eye rests the fine levels and keeps the coarse ones", { timeout: 300_000 }, async () => {
  let frame = apply(session.advance({ ...controls, fly: true }));
  for (let step = 0; step < 240; step++) frame = apply(session.advance({ ...controls, boost: true, x: 1, direction: [0.6, 0.8, 0] }));
  assert.ok(frame.cubes.levels[0].box === null && frame.cubes.levels[0].wanted === 0, "the finest level rests");
  assert.ok(frame.cubes.levels.at(-1).box !== null, "the coarsest level still covers the view");
});
