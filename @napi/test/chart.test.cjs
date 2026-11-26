const { before, test } = require("node:test");
const assert = require("node:assert/strict");
const { spawnSync } = require("node:child_process");
const path = require("node:path");
const { setTimeout: sleep } = require("node:timers/promises");
const { Session } = require("../../.out/addon/derivean.node");

// A young planet and a given place to stand keep the start short; charts need no shore.
const options = { seed: 42, age: 3e8, spawn: [0, 0] };
const controls = {
  elapsed: 1 / 60, x: 0, z: 0, active: true, jump: false, sprint: false,
  direction: [0, 0, -1], radius: 4, shape: "sphere", dig: false, place: false,
  fly: false, boost: false, ascend: 0, teleport: null,
};
const Floats = ["ground", "water", "discharge", "ice", "warmth", "cover"];
const Bytes = ["bedrock", "waters", "level"];

let session;
let info;
let eye;

function wellFormed(chart, request) {
  const pixels = request.size ** 2;
  assert.deepEqual(chart.origin, [request.x - request.span / 2, request.z - request.span / 2]);
  assert.equal(chart.pitch, request.span / request.size);
  assert.equal(chart.size, request.size);
  assert.ok(Number.isInteger(chart.tiles) && chart.tiles >= 0);
  assert.ok(Number.isInteger(chart.finest) && chart.finest >= 0 && chart.finest <= 4 && chart.level.every((level) => level <= chart.finest));
  for (const name of Floats) assert.ok(chart[name] instanceof Float32Array && chart[name].length === pixels, name);
  for (const name of Bytes) assert.ok(chart[name] instanceof Uint8Array && chart[name].length === pixels, name);
  for (const name of [...Floats, ...Bytes]) assert.equal(chart[name].buffer, chart.ground.buffer, `${name} shares the one buffer`);
  assert.ok(chart.ground.every(Number.isFinite) && chart.level.every((level) => level <= 4) && chart.waters.every((kind) => kind <= 3));
  assert.ok(chart.waters.every((kind, pixel) => (kind === 0) === Number.isNaN(chart.water[pixel])), "water has a level exactly where it has a kind");
}

before(() => {
  session = new Session(options);
  info = session.info();
  eye = session.advance(controls).eye;
}, { timeout: 900_000 });

test("a chart is promised at once and read while the session goes on advancing", { timeout: 300_000 }, async () => {
  const request = { x: eye[0], z: eye[2], span: info.planetSpan, size: 1024 };
  let chart;
  let read;
  const asked = performance.now();
  const promise = session.chart(request).then((value) => {
    chart = value;
    read = performance.now() - asked;
  });
  const promised = performance.now() - asked;
  while (!chart) {
    session.advance(controls);
    await sleep(0);
  }
  await promise;
  assert.ok(promised < read / 3, `the call returned after ${promised} ms of the ${read} ms the chart took: it must not read it`);
  wellFormed(chart, request);
  assert.ok(chart.level.every((level) => level === 0), "the whole planet is drawn from the planet");
  assert.ok(chart.waters.every((kind, pixel) => (kind === 1) === (chart.ground[pixel] < info.seaLevel)), "the planet is sea exactly below sea level");
  assert.ok(chart.water.every((level) => Number.isNaN(level) || Math.abs(level - info.seaLevel) < 0.01), "the planet's only water is the sea");
});

test("a chart around the player reads the tiles under them, and the same tiles give the same chart", { timeout: 300_000 }, async () => {
  // The ground tile under the player is built after the start, with the voxels.
  while (session.inspect(eye[0], eye[2]).level !== "ground") {
    session.advance(controls);
    await sleep(50);
  }
  // Half a metre a pixel reads the ground level: what the voxels are cut from and the inspector reads.
  const request = { x: eye[0], z: eye[2], span: 64, size: 128 };
  const chart = await session.chart(request);
  wellFormed(chart, request);
  assert.ok(chart.finest === 4 && chart.level[64 * 128 + 64] === 4, "the player stands on a finished ground tile");
  const record = session.inspect(chart.origin[0] + 64.5 * chart.pitch, chart.origin[1] + 64.5 * chart.pitch);
  assert.equal(record.level, "ground", "the inspector reads the level the chart reads");
  assert.ok(Math.abs(record.ground - chart.ground[64 * 128 + 64]) < 1e-3, "the chart and the inspector agree on the ground");
  // A pixel wider than four ground cells reads the site level, which is all a map at that pitch can show.
  const wide = { x: eye[0], z: eye[2], span: 800, size: 128 };
  const map = await session.chart(wide);
  wellFormed(map, wide);
  assert.ok(map.finest === 3 && map.level[64 * 128 + 64] === 3, "a wide chart reads the site tile under the player");

  const again = await session.chart({ ...request, unless: chart.tiles });
  if (again !== null) assert.ok(again.tiles > chart.tiles, "a chart is only read again when tiles were finished meanwhile");
  assert.equal(await session.chart({ ...request, unless: (again ?? chart).tiles + 1e6 }) === null, false, "another count always gets a chart");
});

test("positions beyond the planet's span are charted and bad requests are refused", async () => {
  const far = { x: eye[0] + 2.5 * info.planetSpan, z: eye[2] - 3.5 * info.planetSpan, span: 4e6, size: 64 };
  wellFormed(await session.chart(far), far);
  const farthest = { x: 1e12, z: -1e12, span: 1e6, size: 16 };
  wellFormed(await session.chart(farthest), farthest);
  // Ground tile indices would not fit 32 bits out there: a chart fine enough for the ground level reads the site level.
  const finest = { x: 1e12, z: -1e12, span: 64, size: 128 };
  const outThere = await session.chart(finest);
  wellFormed(outThere, finest);
  assert.equal(outThere.finest, 3);
  for (const bad of [{ size: 0 }, { size: 4096 }, { span: 0 }, { span: Number.NaN }, { x: Infinity }, { z: "0" }])
    assert.throws(() => session.chart({ ...far, ...bad }), /Chart request/);
  assert.throws(() => session.chart(), /Missing chart request/);
});

test("a world is not torn down under charts that are still being read", { timeout: 900_000 }, async () => {
  const pending = [];
  for (let index = 0; index < 12; index++) pending.push(session.chart({ x: eye[0] + index * 1e5, z: eye[2], span: info.planetSpan, size: 1024 }));
  session.restart(options);
  const charts = await Promise.all(pending);
  assert.ok(charts.every((chart) => chart.ground.every(Number.isFinite)), "charts in flight at a restart were finished on the world they were asked of");
  const request = { x: 0, z: 0, span: 1e6, size: 32 };
  wellFormed(await session.chart(request), request);
});

// Run in a process of its own: the session lives on a worker thread, as it does in the game, and that thread is
// terminated while charts are being read. A world freed under a reader, or a teardown that waits for one that
// never comes, would crash or hang the child.
test("a session thread that is terminated with charts in flight ends cleanly", { timeout: 600_000 }, () => {
  const child = spawnSync(process.execPath, [path.join(__dirname, "chart-teardown.cjs")], { encoding: "utf8", timeout: 540_000 });
  assert.equal(child.signal, null, child.stderr);
  assert.equal(child.status, 0, child.stderr);
});
