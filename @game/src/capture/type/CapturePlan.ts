import type { Frame, WorldOptions } from "../../../contract/GameBridge";
import type { WeatherSought } from "../../clouds/fn/findWeather";

export interface CaptureShot {
  // File stem of the PNG and JSON written for the shot.
  name: string;
  // World metres of the player's feet. A null height stands the player on the ground there (or hovers over
  // water). Omitted leaves the player where the world or the previous shot put them.
  position?: [number, number | null, number];
  // With a null height, or with no position at all (which keeps the player's place): fly this many metres above
  // the ground or water there. Zero stands on it.
  above?: number;
  // Radians. Yaw 0 looks north (-Z) and grows to the west; positive pitch looks up.
  yaw: number;
  pitch: number;
  fly: boolean;
  // Neutral relief shading instead of daylight.
  plain?: boolean;
  // The calendar moment to shoot at: the clock skips ahead to the next such day of the year (0…365) and hour
  // (0…24) and the shot waits for the weather to catch up. `rate` then stays as the clock's rate.
  time?: { dayOfYear?: number; hour?: number; rate?: number };
  // Shoot from the weather cell that best shows this instead of from `position`, whose height still applies.
  // It picks where to look in the simulated weather; it never makes any.
  seek?: WeatherSought;
  // False draws the shot without clouds, for timing them.
  clouds?: boolean;
  // Once the view has settled, fly level along the yaw this long and shoot on arrival. The record then carries
  // the frame times of the flight. Needs `fly`.
  cruise?: { seconds: number; boost?: boolean };
  // Once the view has settled, hold the brush along the view for `frames` frames, digging or (with `place`)
  // setting basalt, `radius` voxels wide, and shoot `seconds` (2 when omitted) after the brush began: for
  // detached pieces, particles, voxel water and the brush on screen. Not with `cruise`.
  brush?: { frames: number; place?: boolean; radius?: number; seconds?: number };
  // Once the view has settled, turn it at this rate (positive turns west, as yaw grows) for `seconds` and shoot while
  // it still turns: for what the picture does in motion. Not with `cruise` or `brush`.
  turn?: { seconds: number; radiansPerSecond: number };
  // Opens the map over the shot, centred on the player, with this many metres across the shorter side of the
  // window; the shot waits for its sharp chart and a cruise flies with the map open. Omitted closes the map.
  map?: { span: number };
  // After the shot is taken, time a frame with the clouds shown and hidden: `batches` (6 when omitted) of `frames`
  // frames drawn back to back each (WorldView.bench). It is written beside the shot as `<name>-bench.json`.
  bench?: { frames: number; batches?: number };
  // Readiness the shot needs to count (Frame.ready): it is not taken on a stall without it, and once its timeout
  // has passed without it, it is taken and recorded as `unready`. Overrides the plan's.
  ready?: Array<keyof Frame["ready"]>;
}

export interface CapturePlan {
  // Omitted starts the engine's default world.
  world?: WorldOptions;
  // Directory for the shots. Like the plan's own path, a relative one starts at the repository root; main/capture.mjs
  // names the one used when absent.
  out?: string;
  // How long the surroundings must stay ready with nothing pending before a shot. A shot is also taken, and marked
  // `stalled`, once the world is playable and pending work has stopped delivering. Defaults: constant/timing.
  settleSeconds?: number;
  // Upper bound on waiting for one shot once the world is playable where it stands; the shot is then taken anyway
  // and marked `timedOut`.
  timeoutSeconds?: number;
  // Window content size in CSS pixels; the window's usual size when absent.
  size?: [number, number];
  // Readiness every shot needs to count, unless the shot says otherwise (CaptureShot.ready). An acceptance plan
  // asks for all four; a stand-in picture then never passes for one.
  ready?: Array<keyof Frame["ready"]>;
  shots: CaptureShot[];
}

export interface CaptureBridge {
  plan(): Promise<CapturePlan>;
  shot(name: string, record: unknown): Promise<void>;
  finish(failure?: string): Promise<void>;
}

declare global {
  interface Window {
    // Present only when the app was started with DERIVEAN_CAPTURE.
    deriveanCapture?: CaptureBridge;
  }
}
