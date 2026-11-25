const { test } = require("node:test");
const assert = require("node:assert/strict");
const { Session, world } = require("../../.out/addon/derivean.node");

const controls = {
  elapsed: 1 / 60, x: 0, z: 0, active: true, jump: false, sprint: false,
  direction: [0, 0, -1], radius: 7, shape: "cube", dig: false, place: false,
};

const balanced = (frame) => {
  const { initial, bulk, airborne, escaped } = frame.water;
  assert.equal(bulk + airborne + escaped, initial);
};

function aim(from, to) {
  const delta = to.map((coordinate, axis) => coordinate - from[axis]);
  const length = Math.hypot(...delta);
  return delta.map((coordinate) => coordinate / length);
}

test("desktop bridge returns two separately meshed water levels and an exact budget", () => {
  const session = new Session();
  const first = session.advance({ ...controls, active: false, elapsed: 0 });
  assert.ok(first.streamingReady);
  assert.ok(first.visibleChunks > 1);
  balanced(first);
  assert.equal(first.water.escaped, 0);
  assert.equal(first.water.airborne, 0);
  assert.equal(world.voxelSize, 0.125);
  assert.equal(world.waterUnitsPerVoxel, 256);
  const levels = new Set(first.waterMeshes.filter(({ faces }) => faces.length).map(({ key }) => key[1]));
  assert.ok(levels.has(-1) && levels.has(0), String([...levels]));
  assert.equal(first.buildStock, 128);
  for (let i = 0; i < 120; i++) {
    const frame = session.advance(controls);
    balanced(frame);
    assert.equal(frame.water.escaped, 0);
  }
});

test("movement, destructive brush, and finite construction reach the native world", () => {
  const session = new Session();
  const start = session.advance({ ...controls, active: false, elapsed: 0 });
  let frame;
  for (let i = 0; i < 36; i++) frame = session.advance({ ...controls, z: -1 });
  assert.ok(frame.eye[2] < start.eye[2] - 2);
  const stock = frame.buildStock;
  const placement = aim(frame.eye, [frame.eye[0] + 1.25, -0.02, frame.eye[2]]);
  frame = session.advance({ ...controls, direction: placement, place: true });
  assert.equal(frame.buildStock, stock - 1);
  assert.equal(world.materials[frame.target?.material]?.name, "Basalt");
  balanced(frame);
  frame = session.advance({ ...controls, direction: [0, -1, 0], dig: true, radius: 3 });
  assert.ok(frame.editedTiles > 0);
  assert.ok(frame.excavatedKg > 0);
  balanced(frame);
});

test("a player opens the upper channel into the lower one, then drains it into the void", () => {
  const session = new Session();
  const step = (command = {}) => session.advance({ ...controls, radius: 4, ...command });
  let frame = step();
  for (let i = 0; i < 57; i++) frame = step({ x: -1 });
  for (let i = 0; i < 59; i++) frame = step({ z: -1 });
  for (let i = 0; i < 12; i++) frame = step();

  const upper = aim(frame.eye, [-4, 2.27, -4.45]);
  assert.equal(world.materials[step({ direction: upper }).target?.material]?.name, "Clay");
  frame = step({ direction: upper, dig: true });
  let peakFalling = frame.water.airborne;
  let sawFoam = false;
  for (let i = 0; i < 360; i++) {
    frame = step();
    balanced(frame);
    peakFalling = Math.max(peakFalling, frame.water.airborne);
    for (let j = 4; j < frame.particles.length; j += 5) sawFoam ||= frame.particles[j] === 2;
  }
  assert.ok(peakFalling > 10_000);
  assert.ok(sawFoam);
  assert.equal(frame.water.escaped, 0);

  const lower = aim(frame.eye, [-4, -0.78, -4.30]);
  assert.equal(world.materials[step({ direction: lower }).target?.material]?.name, "Chalk");
  step({ direction: lower, dig: true });
  for (let i = 0; i < 360; i++) {
    frame = step();
    balanced(frame);
  }
  assert.ok(frame.water.escaped > 100_000);
});

test("a one-voxel breach drains without building water above the upper reservoir", () => {
  const session = new Session();
  const step = (command = {}) => session.advance({ ...controls, radius: 1, ...command });
  let frame = step();
  for (let i = 0; i < 57; i++) frame = step({ x: -1 });
  for (let i = 0; i < 59; i++) frame = step({ z: -1 });
  for (let i = 0; i < 12; i++) frame = step();

  const direction = aim(frame.eye, [-4.025, 2.27, -4.45]);
  assert.equal(world.materials[step({ direction }).target?.material]?.name, "Clay");
  for (let i = 0; i < 4; i++) step({ direction, dig: true });

  let sawWaterBelowBreach = false;
  for (let i = 0; i < 180; i++) {
    frame = step();
    balanced(frame);
    assert.equal(frame.water.escaped, 0);
    for (const { origin, faces } of frame.waterMeshes) {
      for (let j = 0; j < faces.length; j += 8) {
        const [axis, , plane, u, v, width, height] = faces.subarray(j, j + 7);
        const top = axis === 1 ? plane : axis === 0 ? u + width : v + height;
        assert.ok(origin[1] + top * world.voxelSize / world.waterUnitsPerVoxel <= 3,
                  "Water rose above its original source");
      }
    }
    for (let j = 0; j < frame.particles.length; j += 5)
      sawWaterBelowBreach ||= frame.particles[j + 4] === 0 && frame.particles[j + 1] < 1;
  }
  assert.ok(sawWaterBelowBreach, "Water did not fall through the opening");
});

test("a released sand bank settles, including grains over freshly deposited cells", () => {
  const session = new Session();
  let frame;
  for (let i = 0; i < 30; i++) frame = session.advance(controls);
  const direction = aim(frame.eye, [4.75, 0.15, -0.625]);
  frame = session.advance({ ...controls, direction, dig: true, radius: 5 });
  const released = frame.sandAirborne + frame.sandDeposited;
  assert.ok(released > 100);
  for (let i = 0; i < 240; i++) frame = session.advance(controls);
  assert.equal(frame.sandAirborne, 0);
  assert.equal(frame.sandEscaped, 0);
  assert.equal(frame.sandDeposited, released);
});

test("falling through excavated spawn returns to a surviving surface", () => {
  const session = new Session();
  let frame;
  for (let i = 0; i < 30; i++) frame = session.advance(controls);
  const original = frame.eye;
  frame = session.advance({ ...controls, direction: [0, -1, 0], dig: true, radius: world.maxBrushRadius });
  for (let i = 0; i < 240; i++) frame = session.advance(controls);
  assert.ok(Math.hypot(frame.eye[0] - original[0], frame.eye[2] - original[2]) > 1);
  assert.ok(Math.abs(frame.eye[1] - original[1]) < 0.1);
  const recovered = frame.eye;
  for (let i = 0; i < 120; i++) frame = session.advance(controls);
  assert.deepEqual(frame.eye, recovered);
});
