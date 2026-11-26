import assert from "node:assert/strict";
import test from "node:test";
import type { Command, Frame, GameBridge, WorldInfo } from "../contract/GameBridge";
import { createWorldClock } from "../src/calendar/service/WorldClock";
import type { FrameMetrics } from "../src/diagnostics/service/FrameMetrics";
import type { WorldView } from "../src/scene/type/WorldView";
import { createWorldSession } from "../src/session/service/WorldSession";
import type { Pilot } from "../src/control/type/Pilot";

// A stand-in slice: the engine's weather catches up in slices of compute time, each collected and the next begun
// by an advance (engine Weather.cpp).
const Slice = 6 * 3600;

// An engine whose world is streamed in full and whose weather is `lag` seconds behind the calendar.
function settledEngine() {
  let lag = 0;
  const commands: Command[] = [];
  const frame = (): Frame => ({
    streamingReady: true, ready: { site: true, local: true, region: true, far: true }, pending: { chunks: 0, patches: 0 },
    weather: { temperature: 0, rain: 0, snow: 0, wind: [0, 0], cloudCover: 0, humidity: 0, pressure: 1e5, snowpack: 0, lag },
  }) as unknown as Frame;
  const bridge = {
    startWorld: async () => ({}) as WorldInfo,
    advance: async (command: Command) => {
      commands.push(command);
      lag = Math.max(0, lag - Slice) + (command.time?.jump ?? 0);
      return frame();
    },
  } as unknown as GameBridge;
  return { bridge, commands, get lag() { return lag; } };
}

const scene = { unload() {}, load() {}, apply() {}, target() {}, draw() {} } as unknown as WorldView;
const metrics = { reset() {}, advance() {}, frame() {} } as unknown as FrameMetrics;
// The player in the menu: nothing steers.
const menu = {
  active: () => false, yaw: 0, pitch: 0, shape: "sphere", radius: 1,
  command: () => ({ elapsed: 0, active: false }),
} as unknown as Pilot;
const events = { starting() {}, started() {}, frame() {}, failed(error: unknown) { throw error; } };
// Lets the bridge's promises settle between animation frames.
const settle = () => new Promise((resolve) => setImmediate(resolve));

test("a paused world's weather catches up after a skip of a day from the menu", async () => {
  const engine = settledEngine();
  const clock = createWorldClock();
  clock.rate = 0;
  const session = createWorldSession(engine.bridge, scene, metrics, clock, events);
  session.steer(menu);
  session.start();
  let time = 0;
  const frames = async (count: number) => {
    for (let i = 0; i < count; i++) {
      session.tick(time += 16);
      await settle();
    }
  };
  await frames(5);
  const before = engine.commands.length;
  await frames(5);
  assert.equal(engine.commands.length, before, "a settled paused world is still advanced");

  clock.skip(86400);
  await frames(20);
  assert.equal(engine.commands.filter((command) => (command.time?.jump ?? 0) !== 0).length, 1, "the skip is handed over once");
  assert.equal(engine.lag, 0, "the weather stays behind the calendar");
  const caught = engine.commands.length;
  await frames(5);
  assert.equal(engine.commands.length, caught, "the world is advanced after the weather has caught up");
});
