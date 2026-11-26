const { before, test } = require("node:test");
const assert = require("node:assert/strict");
const { setTimeout: sleep } = require("node:timers/promises");
const { Session, world } = require("../../.out/addon/derivean.node");

// Two billion years is about the youngest this seed offers a mainland coast to land on.
const options = { seed: 42, age: 2e9 };
const EyeHeight = 1.62;
const controls = {
  elapsed: 1 / 60, x: 0, z: 0, active: true, jump: false, sprint: false,
  direction: [0, 0, -1], radius: 4, shape: "sphere", dig: false, place: false,
  fly: false, boost: false, ascend: 0, teleport: null, time: { rate: 1, jump: 0 },
};

let session;
let first;
// What a renderer would hold of the far terrain; every frame must keep it consistent.
const patches = new Map();

function applyFar(frame) {
  for (const id of frame.far.removed) assert.ok(patches.delete(id), `patch ${id} was removed but never added`);
  for (const patch of frame.far.added) {
    assert.ok(!patches.has(patch.id), `patch id ${patch.id} was used twice`);
    const vertices = (patch.cells + 1) ** 2;
    assert.equal(patch.cells, world.far.cells);
    assert.ok(patch.heights instanceof Float32Array && patch.heights.length === vertices);
    assert.ok(patch.morph instanceof Float32Array && patch.morph.length === vertices);
    assert.ok(patch.water instanceof Float32Array && patch.water.length === vertices);
    assert.ok(patch.normals instanceof Int8Array && patch.normals.length === vertices * 3);
    assert.ok(patch.colors instanceof Uint8Array && patch.colors.length === vertices * 3);
    assert.equal(patch.size, patch.cells * 2 ** patch.level);
    assert.ok(Number.isInteger(patch.origin[0] / patch.size) && Number.isInteger(patch.origin[2] / patch.size));
    assert.ok(Number.isInteger(patch.origin[1]) && patch.heights.every((height) => height >= 0) && Math.min(...patch.heights) < 1,
              "heights are measured from the patch origin");
    assert.ok([1, 2, 4].includes(patch.shadeDetail) && patch.shade instanceof Uint8Array);
    assert.equal(patch.shade.length, (patch.cells * patch.shadeDetail + 1) ** 2 * 4);
    for (let texel = 2; texel < patch.shade.length; texel += 4) assert.ok(patch.shade[texel] > 0 && patch.shade[texel] < world.materials.length);
    assert.ok(patch.morph.every(Number.isFinite));
    assert.ok(patch.skirt > 0);
    patches.set(patch.id, patch.level);
  }
}

function wellFormed(frame) {
  assert.equal(typeof frame.flying, "boolean");
  assert.equal(frame.cubes, null, "a world of patches sends no cubes");
  for (const mesh of [...frame.chunks, ...frame.waterMeshes, ...frame.spawned])
    assert.ok(mesh.faces instanceof Int16Array && mesh.faces.length % 8 === 0 && !mesh.quads, "a world of patches reads eight words a face");
  assert.equal(typeof frame.generationMs, "number");
  assert.equal(typeof frame.streamingMs, "number");
  for (const level of ["site", "local", "region"]) assert.equal(typeof frame.ready[level], "boolean");
  assert.ok(Number.isInteger(frame.pending.chunks) && Number.isInteger(frame.pending.patches));
  assert.ok(frame.ground === null || Number.isFinite(frame.ground));
  assert.ok(frame.waterLevel === null || Number.isFinite(frame.waterLevel));
  if (frame.voxelBox) {
    for (let axis = 0; axis < 3; axis++) assert.ok(frame.voxelBox.min[axis] < frame.voxelBox.max[axis]);
  }
  const { initial, bulk, airborne, escaped } = frame.water;
  assert.equal(bulk + airborne + escaped, initial);
  assert.ok(frame.particles instanceof Float32Array && frame.particleOrigin.every(Number.isFinite));
  assert.ok(frame.particles.every((value) => Math.abs(value) < 64), "particles are measured from an origin near the eye");

  const { era, seconds, dayOfYear, hour } = frame.calendar;
  assert.ok(era === options.age && seconds >= 0 && dayOfYear >= 0 && dayOfYear < 365 && hour >= 0 && hour < 24);
  assert.ok(Math.abs(Math.hypot(...frame.sun) - 1) < 1e-6);
  const weather = frame.weather;
  for (const key of ["temperature", "rain", "snow", "cloudCover", "humidity", "pressure", "snowpack", "lag"])
    assert.ok(Number.isFinite(weather[key]), `weather.${key}`);
  assert.ok(weather.wind.length === 2 && weather.cloudCover <= 1 && weather.humidity <= 1 && weather.lag >= 0);
  if (frame.clouds) {
    const { cells, cell, origin, time, bands } = frame.clouds;
    assert.ok(cells > 0 && cell > 0 && origin.length === 2 && time <= seconds && bands.length === 3);
    for (const band of bands) {
      for (const channel of ["cover", "base", "top", "depth", "ice", "windX", "windZ"])
        assert.ok(band[channel] instanceof Float32Array && band[channel].length === cells * cells, channel);
      assert.ok(band.genus instanceof Uint8Array && band.genus.length === cells * cells && band.genus.every((genus) => genus < 10));
    }
    for (const channel of ["heaping", "rain", "snow", "snowpack", "snowLine"]) assert.ok(frame.clouds[channel] instanceof Float32Array && frame.clouds[channel].length === cells * cells);
    assert.ok(world.snowLadder.length > 1 && world.snowLadder.every((height, rung) => rung === 0 || height > world.snowLadder[rung - 1]));
    assert.ok(frame.clouds.snowLadder instanceof Float32Array && frame.clouds.snowLadder.length === cells * cells * world.snowLadder.length);
    assert.ok(Number.isFinite(frame.clouds.seaLevel) && frame.clouds.snowLadder.every((snow) => snow >= 0));
  }
}

function step(command = {}) {
  const frame = session.advance({ ...controls, ...command });
  wellFormed(frame);
  applyFar(frame);
  return frame;
}

// Background work needs wall-clock time, not frames.
async function until(done, command = {}) {
  for (let waited = 0; waited < 1_200_000; waited += 5) {
    const frame = step(command);
    if (done(frame)) return frame;
    await sleep(5);
  }
  throw new Error("The world never got there");
}

const settled = (frame) => frame.streamingReady && frame.voxelBox !== null && frame.pending.chunks === 0;
const feetAt = (eye, offset = [0, 0, 0]) => eye.map((value, axis) => value + offset[axis] - (axis === 1 ? EyeHeight : 0));

function aim(from, to) {
  const delta = to.map((coordinate, axis) => coordinate - from[axis]);
  const length = Math.hypot(...delta);
  return delta.map((coordinate) => coordinate / length);
}

before(async () => {
  session = new Session(options);
  first = step({ active: false, elapsed: 0 });
  await until(settled);
}, { timeout: 2_400_000 });

test("the world describes itself and arrives around a player who waits for it", () => {
  const info = session.info();
  assert.deepEqual([info.seed, info.age, info.worldFloor], [options.seed, options.age, -8192]);
  assert.ok(Number.isFinite(info.seaLevel));
  assert.equal(world.voxelSize, 0.125);
  assert.equal(world.waterUnitsPerVoxel, 256);
  assert.ok(world.materials.length >= 37 && world.materials.some(({ name }) => name === "Basalt"));
  assert.ok(!first.streamingReady && first.voxelBox === null, "the first frame cannot have voxels yet");
  assert.equal(first.buildStock, 128);

  let frame;
  for (let i = 0; i < 60; i++) frame = step();
  assert.ok(frame.visibleChunks > 100 && patches.size > 0);
  assert.ok(frame.voxelBox.min[0] < frame.eye[0] && frame.eye[0] < frame.voxelBox.max[0]);
  assert.ok(Math.abs(frame.eye[1] - EyeHeight - frame.ground) < 0.4, "the player stands on the simulated ground");
  assert.ok(Math.hypot(frame.eye[0] - first.eye[0], frame.eye[2] - first.eye[2]) < 1, "waiting keeps the player where the world put them");
  assert.equal(frame.water.initial, 0);

  const record = session.inspect(frame.eye[0], frame.eye[2]);
  assert.equal(record.level, "site");
  assert.ok(Math.abs(record.ground - frame.ground) < 1e-3 && typeof record.bedrock === "string");
});

test("movement, destructive brush, and finite construction reach the native world", () => {
  let frame = step();
  const start = frame.eye;
  for (let i = 0; i < 36; i++) frame = step({ z: -1 });
  assert.ok(frame.eye[2] < start[2] - 2, "walking moves the player");
  const stock = frame.buildStock;
  const placement = aim(frame.eye, [frame.eye[0] + 1.25, frame.ground, frame.eye[2]]);
  frame = step({ direction: placement, place: true });
  assert.equal(frame.buildStock, stock - 1);
  assert.equal(world.materials[frame.target?.material]?.name, "Basalt");
  frame = step({ direction: [0, -1, 0], dig: true, radius: 3 });
  assert.ok(frame.editedTiles > 0 && frame.excavatedKg > 0);
  assert.ok(frame.chunks.length > 0, "an edit re-meshes the chunks it touched");
});

test("a buried player is lifted to the surface and a lost one starts over", () => {
  let frame = step();
  frame = step({ teleport: feetAt(frame.eye, [8, 0, 8]) });
  for (let i = 0; i < 60; i++) frame = step();
  // Wherever the body came to rest on the rough ground, a metre under that place is inside the ground.
  const stood = feetAt(frame.eye);
  step({ teleport: [stood[0], stood[1] - 1, stood[2]] });
  frame = step();
  assert.ok(Math.hypot(frame.eye[0] - stood[0], frame.eye[2] - stood[2]) < 0.5, "a buried player moves straight up");
  assert.ok(Math.abs(frame.eye[1] - EyeHeight - stood[1]) < 0.5, "a buried player ends up standing on the surface");

  step({ teleport: [stood[0], session.info().worldFloor - 50, stood[2]] });
  frame = step();
  assert.ok(Math.hypot(frame.eye[0] - first.eye[0], frame.eye[2] - first.eye[2]) < 1e-3 && Math.abs(frame.eye[1] - first.eye[1]) < 1,
            "below the world floor the player returns to the spawn");
});

test("flight leaves the ground behind, and the voxels it left come back", { timeout: 900_000 }, async () => {
  let frame = await until(settled);
  const home = feetAt(frame.eye);
  frame = step({ fly: true });
  assert.ok(frame.flying);
  const low = frame.eye[1];
  for (let i = 0; i < 120; i++) frame = step({ ascend: 1 });
  assert.ok(frame.eye[1] - low > 20 && frame.eye[1] - low < 28, "flight climbs at about 12 m/s");
  for (let i = 0; i < 120; i++) frame = step({ ascend: 1, boost: true });
  assert.ok(frame.eye[1] - low > 500, "boosted flight leaves the ground behind");
  frame = await until((next) => next.visibleChunks === 0);
  assert.equal(frame.voxelBox, null, "voxels far below the player are dropped");

  step({ teleport: home });
  frame = step({ fly: true });
  assert.ok(!frame.flying);
  frame = await until(settled);
  assert.equal(frame.water.escaped + frame.sandEscaped + frame.debrisEscapedKg, 0, "nothing escaped while the ground was away");
});

test("the calendar drives the sun, the weather and the clouds", { timeout: 300_000 }, async () => {
  let frame = step();
  const { seconds, dayOfYear } = frame.calendar;
  frame = step({ time: { rate: 0, jump: 86400 } });
  assert.equal(frame.calendar.seconds, seconds + 86400);
  assert.ok(Math.abs(((frame.calendar.dayOfYear - dayOfYear + 365) % 365) - 1) < 1e-6, "a day's jump turns the calendar a day");
  assert.ok(frame.weather.lag > 0, "the atmosphere follows a jump in the background");
  assert.equal(step({ time: { rate: 0, jump: 0 } }).calendar.seconds, seconds + 86400, "at rate zero the calendar stands still");

  const height = frame.sun[1];
  let snapshots = 0;
  for (let i = 0; i < 360; i++) {
    frame = step({ time: { rate: 3600, jump: 0 } });
    snapshots += frame.clouds !== null;
    await sleep(5);
  }
  assert.ok(Math.abs(frame.calendar.seconds - seconds - 86400 - 6 * 3600) < 1e-6, "six seconds at an hour a second");
  assert.ok(Math.abs(frame.sun[1] - height) > 0.2, "six hours move the sun");
  assert.ok(snapshots >= 1, "cloud snapshots arrive while the atmosphere steps");
  frame = await until((next) => next.weather.lag === 0, { time: { rate: 0, jump: 0 } });

  const record = session.inspect(frame.eye[0], frame.eye[2]);
  assert.equal(typeof record["Weather / Why"], "string");
  assert.ok(record["Weather / Planet water kg"] > 0 && Number.isFinite(record["Weather / Air temperature °C"]));
});

test("malformed input is refused before it reaches the world", () => {
  assert.throws(() => new Session({ seed: -1, age: 1e9 }), RangeError);
  assert.throws(() => new Session({ seed: 1 }), TypeError);
  assert.throws(() => session.advance({ ...controls, fly: undefined }), TypeError);
  assert.throws(() => session.advance({ ...controls, teleport: [0, Infinity, 0] }), RangeError);
  assert.throws(() => session.advance({ ...controls, time: { rate: -1, jump: 0 } }), RangeError);
  assert.throws(() => session.advance({ ...controls, time: 3600 }), TypeError);
});
