import type { Frame, GameBridge, WorldInfo, WorldOptions } from "../../../contract/GameBridge";
import type { WorldClock } from "../../calendar/service/WorldClock";
import type { Pilot } from "../../control/type/Pilot";
import type { FrameMetrics } from "../../diagnostics/service/FrameMetrics";
import type { WorldView } from "../../scene/type/WorldView";
import { worldSettled } from "../fn/worldSettled";

export interface SessionEvents {
  // A world is being generated; no frames arrive until `started`.
  starting(options: WorldOptions | undefined): void;
  started(world: WorldInfo): void;
  frame(frame: Frame, world: WorldInfo): void;
  failed(error: unknown): void;
}

// One world at a time: starts it, exchanges one in-flight command for one frame, and draws. A restart drops the
// frames of the world it replaces.
export function createWorldSession(
  bridge: GameBridge, scene: WorldView, metrics: FrameMetrics, clock: WorldClock,
  events: SessionEvents,
) {
  let pilot: Pilot | undefined;
  let world: WorldInfo | undefined;
  let frame: Frame | undefined;
  let epoch = 0;
  let queue: Promise<void> = Promise.resolve();
  let inFlight = false;
  let visited = false;
  let failed = false;
  let lastDraw = performance.now();
  let lastAdvance = lastDraw;

  function fail(error: unknown) {
    if (failed) return;
    failed = true;
    events.failed(error);
  }

  function start(options?: WorldOptions) {
    const mine = ++epoch;
    failed = false;
    world = undefined;
    frame = undefined;
    visited = false;
    scene.unload();
    events.starting(options);
    // Starts run one after another and only the latest counts: an earlier one neither loads nor reports failure.
    queue = queue.then(async () => {
      if (mine !== epoch) return;
      const next = await bridge.startWorld(options);
      if (mine !== epoch) return;
      world = next;
      scene.load(next);
      metrics.reset(next);
      events.started(next);
    }).catch((error: unknown) => {
      if (mine === epoch) fail(error);
    });
  }

  function advance(time: number, info: WorldInfo, by: Pilot) {
    const mine = epoch;
    const command = { ...by.command(Math.max(0, (time - lastAdvance) / 1000), time, frame), time: clock.take() };
    const dispatched = performance.now();
    lastAdvance = time;
    inFlight = true;
    void bridge.advance(command).then((next) => {
      if (mine !== epoch || failed) return;
      frame = next;
      scene.apply(next);
      if (command.active) metrics.advance(next, performance.now() - dispatched, command.elapsed);
      events.frame(next, info);
    }).catch((error: unknown) => {
      if (mine === epoch) fail(error);
    }).finally(() => { inFlight = false; });
  }

  // One animation frame: returns what is on screen and who steers it, if anything is.
  function tick(time: number) {
    const dt = time - lastDraw;
    lastDraw = time;
    if (failed || !pilot) return undefined;
    const active = pilot.active();
    if (active) visited = true;
    if (active && dt > 0) metrics.frame(dt);
    // The engine streams only when asked, so a paused world is still advanced until it has settled, and once
    // more for a skip of the clock asked for from the launcher.
    if (world && !inFlight && (active || !frame || !worldSettled(frame) || clock.skipping)) advance(time, world, pilot);
    else if (!active && !inFlight) lastAdvance = time;
    if (!world) return undefined;
    scene.target(active ? frame?.target?.voxel : undefined, pilot.shape, pilot.radius);
    scene.draw(visited, pilot.yaw, pilot.pitch, Math.max(0, dt) / 1000, time / 1000);
    return frame ? { world, frame, active, pilot } : undefined;
  }

  return {
    start,
    tick,
    steer(next: Pilot) { pilot = next; },
    get visited() { return visited; },
  };
}
