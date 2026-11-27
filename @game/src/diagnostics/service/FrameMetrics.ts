import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { readClock, readRate } from "../../calendar/fn/readClock";
import { formatAge } from "../../units/fn/age";
import { distance, metres } from "../../units/fn/length";
import { summary, type Summary } from "../fn/summary";

const Window = 600;

export interface ViewStats {
  // `levels`: chunks held by each level of cubes.
  calls: number; triangles: number; levels: number[]; yaw: number; pitch: number;
  // Clock rate asked for, and milliseconds of GPU time the clouds take (null while not drawn or untimed).
  rate: number; cloudMs: number | null;
  // What draws the world: WebGPU, or "starting" until the device is there.
  backend: string;
  // Milliseconds the draw takes on the CPU and on the GPU; null where that is not measured.
  draw: { cpuMs: number | null; gpuMs: number | null };
}

export function createFrameMetrics(output: HTMLOutputElement) {
  const frameTimes: number[] = [];
  const bridgeTimes: number[] = [];
  const engineTimes: number[] = [];
  let lap: number[] | undefined;
  let world: WorldInfo | undefined;
  let lastEye: Frame["eye"] | undefined;
  let speed = 0;
  let last = 0;

  function record(series: number[], value: number) {
    series.push(value);
    if (series.length > Window) series.shift();
  }

  const ms = (value: number) => value.toFixed(1);
  // Compass degrees the wind blows towards: 0 north (-Z), 90 east (+X).
  const bearing = ([x, z]: [number, number]) => ((Math.round(Math.atan2(x, -z) * 180 / Math.PI) % 360) + 360) % 360;
  const flag = (ready: boolean) => ready ? "ready" : "loading";

  function lines(frame: Frame, info: WorldInfo, view: ViewStats) {
    const liters = (units: number) => (units / info.waterUnitsPerVoxel * info.voxelSize ** 3 * 1000).toFixed(1);
    const frames = summary(frameTimes);
    const { bulk, airborne, escaped, initial } = frame.water;
    const [x, y, z] = frame.eye;
    const { weather } = frame;
    return [
      `${frames.mean ? Math.round(1000 / frames.mean) : 0} FPS   frame ${ms(frames.mean)} mean · ${ms(frames.p95)} p95 · ${ms(frames.p99)} p99 · ${ms(frames.max)} max ms`,
      `${ms(summary(engineTimes).p95)} ms C++   ${ms(summary(bridgeTimes).p95)} ms round trip p95`,
      `Streaming  ${ms(frame.streamingMs)} ms on the game thread · ${ms(frame.generationMs)} ms generation behind it`,
      `${view.backend}   draw ${view.draw.cpuMs === null ? "—" : ms(view.draw.cpuMs)} ms CPU · ${view.draw.gpuMs === null ? "—" : ms(view.draw.gpuMs)} ms GPU`,
      `${view.calls} draws   ${view.triangles.toLocaleString("en-US")} triangles   clouds ${view.cloudMs === null ? "—" : `${ms(view.cloudMs)} ms GPU`}`,
      `${frame.visibleChunks} chunks   ${frame.editedTiles} edited m³ tiles   ${frame.bodies} ${frame.bodies === 1 ? "body" : "bodies"}`,
      `World  seed ${info.seed} · ${formatAge(info.age)} · sea level ${info.seaLevel.toFixed(1)} m`,
      `LOD  site ${flag(frame.ready.site)} · local ${flag(frame.ready.local)} · region ${flag(frame.ready.region)}`,
      `Pending  ${frame.pending.chunks} chunks · ${frame.pending.patches} patches`,
      `Cube chunks  ${view.levels.reduce((sum, count) => sum + count, 0)}   by level ${view.levels.join(" ") || "—"}`,
      `X ${metres(x)}   Y ${metres(y)}   Z ${metres(z)}`,
      `Ground  ${frame.ground === null ? "—" : `${metres(frame.ground)} m · eye ${distance(y - frame.ground)} above`}`
        + `   Water level  ${frame.waterLevel === null ? "—" : `${metres(frame.waterLevel)} m`}`,
      `${frame.flying ? "Flying" : "Walking"}  ${speed >= 100 ? speed.toFixed(0) : speed.toFixed(1)} m/s`
        + `   Look ${view.yaw.toFixed(3)}  ${view.pitch.toFixed(3)} rad`,
      `${readClock(frame.calendar)} · ${readRate(view.rate)}`
        + (weather.lag > 0 ? ` · weather ${weather.lag >= 3600 ? `${(weather.lag / 3600).toFixed(1)} h` : `${weather.lag.toFixed(0)} s`} behind` : ""),
      `Weather  ${weather.temperature.toFixed(1)} °C · rain ${weather.rain.toFixed(1)} · snow ${weather.snow.toFixed(1)} mm/h`
        + ` · wind ${Math.hypot(...weather.wind).toFixed(1)} m/s to ${bearing(weather.wind)}°`,
      `Air  cloud ${(weather.cloudCover * 100).toFixed(0)} % · humidity ${(weather.humidity * 100).toFixed(0)} %`
        + ` · ${(weather.pressure / 100).toFixed(0)} hPa · snowpack ${weather.snowpack.toFixed(1)} mm`,
      // The ledgers of voxel water and sand appear once there is something in them.
      ...(initial + bulk + airborne + escaped > 0 ? [
        `Voxel water  ${liters(bulk)} L pooled · ${liters(airborne)} L falling · ${liters(escaped)} L escaped`,
        `Voxel water balance  ${liters(bulk + airborne + escaped)} / ${liters(initial)} L`,
      ] : []),
      ...(frame.sandAirborne + frame.sandDeposited + frame.sandEscaped > 0
        ? [`Sand  ${frame.sandAirborne} falling · ${frame.sandDeposited} settled · ${frame.sandEscaped} escaped`] : []),
      `Rock  ${frame.excavatedKg.toFixed(1)} kg cut · ${frame.debrisEscapedKg.toFixed(1)} kg escaped`,
      ...(frame.waterDepth !== null ? [`Depth  ${frame.waterDepth.toFixed(2)} m`] : []),
    ];
  }

  return {
    reset(next: WorldInfo) {
      world = next;
      lastEye = undefined;
      speed = 0;
      frameTimes.length = bridgeTimes.length = engineTimes.length = 0;
    },
    frame(dt: number) {
      record(frameTimes, dt);
      lap?.push(dt);
    },
    // `elapsed` is the simulated time the frame covers, in seconds.
    advance(frame: Frame, roundTrip: number, elapsed: number) {
      record(engineTimes, frame.nativeMs);
      record(bridgeTimes, roundTrip);
      if (lastEye && elapsed > 0) {
        const moved = Math.hypot(frame.eye[0] - lastEye[0], frame.eye[1] - lastEye[1], frame.eye[2] - lastEye[2]);
        speed += (moved / elapsed - speed) * 0.2;
      }
      lastEye = frame.eye;
    },
    get speed() { return speed; },
    frames: (): Summary => summary(frameTimes),
    roundTrips: (): Summary => summary(bridgeTimes),
    // Frame times between startLap and endLap, for scripted measurements.
    startLap() { lap = []; },
    endLap(): Summary {
      const result = summary(lap ?? []);
      lap = undefined;
      return result;
    },
    paint(now: number, frame: Frame, view: ViewStats) {
      if (output.hidden || !world || now - last < 300) return;
      last = now;
      output.value = lines(frame, world, view).join("\n");
    },
  };
}

export type FrameMetrics = ReturnType<typeof createFrameMetrics>;
