// 272 lines: one state machine over a shot's phases, which share the shot's bookkeeping; split by phase, that
// state would pass through a layer that only forwards it.
import type { Frame, Point } from "../../../contract/GameBridge";
import type { WorldClock } from "../../calendar/service/WorldClock";
import type { WeatherSought } from "../../clouds/fn/findWeather";
import type { Pilot, Steering } from "../../control/type/Pilot";
import { worldSettled } from "../../session/fn/worldSettled";
import { PlayableSeconds, SettleSeconds, StallSeconds, TimeoutSeconds } from "../constant/timing";
import { until } from "../fn/until";
import type { CaptureBridge, CapturePlan, CaptureShot } from "../type/CapturePlan";

// Height from which the ground under a shot is looked up before standing on it.
const ProbeHeight = 12_000;
// The teleport places the feet; drop onto the ground from just above it.
const StandClearance = 1;
const HoverHeight = 1.7;
const StillMetres = 0.002;
// A reading of the surface taken from afar is coarse. Stand again when the one taken on arrival differs by more.
const SurfaceTolerance = 0.5;
const Refinements = 2;

export interface CaptureHost {
  clock: WorldClock;
  setPlain(plain: boolean): void;
  setClouds(shown: boolean): void;
  // World x and z of the weather cell that best shows what is sought, once a snapshot has arrived.
  findWeather(sought: WeatherSought): [number, number] | undefined;
  startLap(): void;
  // Opens the map at a span in metres or, without one, closes it; and whether the map has nothing left to load.
  setMap(span: number | undefined): void;
  mapSettled(): boolean;
  // Renderer-side numbers for the shot record; `lap` closes the frame-time lap opened by startLap.
  measure(lap: boolean): Record<string, unknown>;
  // GPU time of a frame with the clouds shown and hidden, drawn back to back.
  bench(frames: number, batches?: number): Promise<unknown>;
  // How much of the ground round a point the simulation holds at each level of detail, as shares.
  survey(x: number, z: number): Promise<Record<string, number>>;
}

type Phase = "time" | "place" | "ground" | "settle" | "cruise" | "brush" | "turn" | "save";

// Calendar seconds behind the clock at which the weather counts as caught up.
const Caught = 60;

const surfaceOf = (frame: Frame, ground: number) => Math.max(ground, frame.waterLevel ?? -Infinity);

// Steers the player through the plan's shots without pointer lock: teleport, wait for the surroundings, shoot.
// The engine applies a flight toggle in the frame that carries it, so each command can ask for one whenever the
// last frame disagrees with the wanted state.
export function createCaptureDriver(bridge: CaptureBridge, plan: CapturePlan, host: CaptureHost): Pilot {
  const settle = (plan.settleSeconds ?? SettleSeconds) * 1000;
  const timeout = (plan.timeoutSeconds ?? TimeoutSeconds) * 1000;
  let index = 0;
  let phase: Phase = "time";
  let began = 0;
  let sought: [number, number] | undefined;
  let stood = "";
  let skipped = false;
  let since = 0;
  let quiet: number | undefined;
  let wantFlight = false;
  let surface: number | undefined;
  let refined = 0;
  let lastHeight = Number.NaN;
  let lastArrival = 0;
  // When the world was first playable where this shot stands: its timeout counts from then (PlayableSeconds), so a
  // slow start is waited out while the tiles that keep finishing after it are not.
  let playable: number | undefined;
  let finished = "";
  let notes: Record<string, unknown> = {};
  let cruise: { from: Point; pendingChunks: number; pendingPatches: number; generationMs: number } | undefined;
  let strokes = 0;
  // When the view of a turning shot began to turn.
  let turning: number | undefined;
  // Where the shot looks: a turning shot keeps turning until it is taken.
  const viewYaw = () => {
    const current = shot();
    const turned = turning === undefined || !current?.turn ? 0 : (performance.now() - turning) / 1000 * current.turn.radiansPerSecond;
    return (current?.yaw ?? 0) + turned;
  };

  const shot = (): CaptureShot | undefined => plan.shots[index];

  function enter(next: Phase, time: number) {
    phase = next;
    since = time;
    quiet = undefined;
  }

  // Where the feet go for a shot without a height, from the frame's reading of the ground and water there.
  function stand(current: CaptureShot, frame: Frame, ground: number, x: number, z: number): Point {
    surface = surfaceOf(frame, ground);
    const afloat = surface > ground;
    const hover = current.above !== undefined && current.above > 0;
    wantFlight = current.fly || afloat || hover;
    notes.hoveringOverWater = afloat && !current.fly && !hover ? true : undefined;
    return [x, surface + (hover ? current.above! : afloat ? HoverHeight : StandClearance), z];
  }

  function save(current: CaptureShot, frame: Frame, extra: Record<string, unknown>) {
    phase = "save";
    const record = {
      name: current.name, eye: frame.eye, yaw: viewYaw(), pitch: current.pitch, flying: frame.flying,
      calendar: frame.calendar, sun: frame.sun, weather: frame.weather,
      ground: frame.ground, waterLevel: frame.waterLevel, streamingReady: frame.streamingReady, ready: frame.ready, tiles: frame.tiles,
      pending: frame.pending, generationMs: frame.generationMs, nativeMs: frame.nativeMs,
      visibleChunks: frame.visibleChunks, ...notes, ...extra,
    };
    void (async () => {
      const dataLevels = await host.survey(frame.eye[0], frame.eye[2]);
      for (let i = 0; i < 3; i++) await new Promise(requestAnimationFrame);
      await bridge.shot(current.name, { ...record, dataLevels });
      if (current.bench) await bridge.shot(`${current.name}-bench`, await host.bench(current.bench.frames, current.bench.batches));
      index++;
      notes = {};
      turning = undefined;
      phase = "time";
      skipped = false;
      if (!shot()) await bridge.finish();
    })().catch((error: unknown) => bridge.finish(String(error)));
  }

  function command(elapsed: number, time: number, frame: Frame | undefined): Steering {
    const current = shot();
    const yaw = viewYaw();
    const pitch = current?.pitch ?? 0;
    const next: Steering = {
      elapsed: Math.min(elapsed, 0.1), x: 0, z: 0, jump: false, sprint: false, active: true,
      direction: [-Math.sin(yaw) * Math.cos(pitch), Math.sin(pitch), -Math.cos(yaw) * Math.cos(pitch)],
      radius: 1, shape: "sphere", dig: false, place: false, fly: false, boost: false, ascend: 0, teleport: null,
    };
    if (!current || !frame) return next;
    // A tile finished anywhere counts as an arrival as much as a chunk: the view is still coming in.
    const tiles = Object.values(frame.tiles).map((tile) => tile.finished).join();
    if (tiles !== finished || frame.chunks.length || frame.far.added.length || frame.far.removed.length || frame.cubes?.added.length) lastArrival = time;
    finished = tiles;
    if (phase === "time") {
      // Skip the clock first, then wait for the weather of that moment before choosing where to stand in it.
      if (!skipped) {
        skipped = true;
        began = time;
        sought = undefined;
        if (current.time) {
          host.clock.skip(until(frame.calendar, current.time.dayOfYear, current.time.hour));
          if (current.time.rate !== undefined) host.clock.rate = current.time.rate;
        }
        return next;
      }
      const late = time - began > timeout;
      if (!late && (host.clock.skipping || frame.weather.lag > Caught)) return next;
      if (late && frame.weather.lag > Caught) notes.weatherLagged = true;
      if (current.seek) {
        sought = host.findWeather(current.seek);
        if (!sought && !late) return next;
        if (!sought) notes.weatherNotFound = true;
      }
      began = time;
      playable = undefined;
      phase = "place";
    }
    // Without a position a shot stays over where the player is; a height above ground then still applies.
    const here: [number, number | null, number] = [frame.eye[0], current.above === undefined ? frame.eye[1] : null, frame.eye[2]];
    const [x, y, z] = sought ? [sought[0], current.position?.[1] ?? null, sought[1]] : current.position ?? here;
    const lost = time - Math.min(playable ?? Infinity, began + PlayableSeconds * 1000) > timeout;

    if (phase === "place") {
      host.setPlain(current.plain ?? false);
      host.setClouds(current.clouds ?? true);
      host.setMap(current.map?.span);
      // A shot from where the last one stood only turns its head.
      const spot = JSON.stringify([current.position ?? null, current.above ?? null, sought ?? null, current.fly]);
      if (spot === stood && !current.position) enter("settle", time);
      else {
        stood = spot;
        if (current.position || sought || y === null) next.teleport = [x, y ?? ProbeHeight, z];
        wantFlight = current.fly || y === null;
        surface = undefined;
        refined = 0;
        enter(y === null ? "ground" : "settle", time);
      }
    } else if (phase === "ground") {
      const arrived = Math.abs(frame.eye[0] - x) < 1 && Math.abs(frame.eye[2] - z) < 1;
      if (arrived && frame.ground !== null) {
        next.teleport = stand(current, frame, frame.ground, x, z);
        enter("settle", time);
      } else if (lost) {
        notes.groundUnknown = true;
        enter("settle", time);
      }
    } else if (phase === "settle") {
      const posed = frame.flying === wantFlight && (frame.flying || Math.abs(frame.eye[1] - lastHeight) < StillMetres);
      if (frame.streamingReady && posed) playable ??= time;
      if (worldSettled(frame) && posed && host.mapSettled()) quiet ??= time;
      else quiet = undefined;
      const settled = quiet !== undefined && time - quiet >= settle;
      const quietTiles = quiet === undefined && frame.streamingReady && posed
        && time - Math.max(since, lastArrival) > StallSeconds * 1000;
      // A shot that asks for levels of readiness is not taken on a stall without them: it waits them out to its
      // timeout and is then taken as lacking them. It still stands again on a stall.
      const unready = (current.ready ?? plan.ready ?? []).filter((level) => !frame.ready[level]);
      const stalled = quietTiles && unready.length === 0;
      const moved = surface !== undefined && frame.ground !== null
        && Math.abs(surfaceOf(frame, frame.ground) - surface) > SurfaceTolerance;
      if ((settled || quietTiles) && !lost && moved && refined < Refinements && frame.ground !== null) {
        refined++;
        next.teleport = stand(current, frame, frame.ground, x, z);
        enter("settle", time);
      } else if (settled || stalled || lost) {
        if (!settled) notes[stalled ? "stalled" : "timedOut"] = true;
        notes.outcome = settled ? "settled" : stalled ? "stalled" : "timedOut";
        if (unready.length) notes.unready = unready;
        notes.waitedSeconds = Number(((time - began) / 1000).toFixed(1));
        if (current.cruise) {
          cruise = { from: frame.eye, pendingChunks: 0, pendingPatches: 0, generationMs: 0 };
          host.startLap();
          enter("cruise", time);
        } else if (current.brush) {
          strokes = 0;
          enter("brush", time);
        } else if (current.turn) {
          turning = time;
          enter("turn", time);
        } else {
          save(current, frame, host.measure(false));
        }
      }
    } else if (phase === "cruise" && current.cruise && cruise) {
      // Level flight along the yaw, whatever the camera's pitch.
      next.x = -Math.sin(yaw);
      next.z = -Math.cos(yaw);
      next.direction = [next.x, 0, next.z];
      next.boost = current.cruise.boost ?? false;
      cruise.pendingChunks = Math.max(cruise.pendingChunks, frame.pending.chunks);
      cruise.pendingPatches = Math.max(cruise.pendingPatches, frame.pending.patches);
      cruise.generationMs = Math.max(cruise.generationMs, frame.generationMs);
      if (time - since >= current.cruise.seconds * 1000) {
        const seconds = (time - since) / 1000;
        const travelled = Math.hypot(...frame.eye.map((value, axis) => value - cruise!.from[axis]));
        save(current, frame, {
          ...host.measure(true),
          cruise: {
            seconds: Number(seconds.toFixed(2)), boost: next.boost, metres: Math.round(travelled),
            metresPerSecond: Math.round(travelled / seconds), maxPendingChunks: cruise.pendingChunks,
            maxPendingPatches: cruise.pendingPatches, maxGenerationMs: cruise.generationMs,
          },
        });
      }
    } else if (phase === "brush" && current.brush) {
      const { frames, place = false, radius = 1, seconds = 2 } = current.brush;
      next.radius = radius;
      if (strokes < frames) {
        strokes++;
        next.dig = !place;
        next.place = place;
      } else if (time - since >= seconds * 1000) {
        save(current, frame, { ...host.measure(false), brush: { strokes, place, radius, seconds } });
      }
    } else if (phase === "turn" && current.turn && time - since >= current.turn.seconds * 1000) {
      save(current, frame, { ...host.measure(false), turn: current.turn });
    }
    if (phase === "ground" || phase === "settle") next.fly = frame.flying !== wantFlight;
    lastHeight = frame.eye[1];
    return next;
  }

  return {
    command,
    active: () => true,
    get yaw() { return viewYaw(); },
    get pitch() { return shot()?.pitch ?? 0; },
    // The brush shown is the one the shot holds.
    get radius() { return shot()?.brush?.radius ?? 1; },
    shape: "sphere",
  };
}
