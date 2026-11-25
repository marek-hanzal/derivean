import type { Frame, WorldInfo } from "../../../contract/GameBridge";

export function createFrameMetrics(output: HTMLOutputElement, world: WorldInfo) {
  const liters = (units: number) => (units / world.waterUnitsPerVoxel * world.voxelSize ** 3 * 1000).toFixed(1);
  const frameTimes: number[] = [];
  const bridgeTimes: number[] = [];
  const engineTimes: number[] = [];
  let last = 0;

  function record(series: number[], value: number) {
    series.push(value);
    if (series.length > 180) series.shift();
  }

  function p95(series: number[]) {
    const sorted = series.slice().sort((a, b) => a - b);
    return (sorted[Math.max(0, Math.ceil(sorted.length * 0.95) - 1)] ?? 0).toFixed(1);
  }

  return {
    frame(dt: number) { record(frameTimes, dt); },
    advance(frame: Frame, roundTrip: number) {
      record(engineTimes, frame.nativeMs);
      record(bridgeTimes, roundTrip);
    },
    paint(now: number, frame: Frame, drawCalls: number, triangles: number, yaw: number, pitch: number) {
      if (output.hidden || now - last < 300) return;
      last = now;
      const fps = Math.round(1000 * frameTimes.length / frameTimes.reduce((a, b) => a + b, 0));
      const cell = frame.eye.map((coordinate) => coordinate.toFixed(2)).join("  ");
      const { bulk, airborne, escaped, initial } = frame.water;
      output.value = [
        `${fps} FPS   ${p95(frameTimes)} ms frame p95`,
        `${p95(engineTimes)} ms C++   ${p95(bridgeTimes)} ms round trip p95`,
        `${frame.visibleChunks} chunks   ${frame.editedTiles} edited m³ tiles   ${frame.bodies} ${frame.bodies === 1 ? "body" : "bodies"}`,
        `${drawCalls} draws   ${triangles.toLocaleString()} triangles`,
        `Eye  ${cell}   Look ${yaw.toFixed(3)}  ${pitch.toFixed(3)} rad`,
        `Water  ${liters(bulk)} L pooled · ${liters(airborne)} L falling · ${liters(escaped)} L escaped`,
        `Water balance  ${liters(bulk + airborne + escaped)} / ${liters(initial)} L`,
        `Sand  ${frame.sandAirborne} falling · ${frame.sandDeposited} settled · ${frame.sandEscaped} escaped`,
        `Rock  ${frame.excavatedKg.toFixed(1)} kg cut · ${frame.debrisEscapedKg.toFixed(1)} kg escaped`,
        ...(frame.waterDepth !== null ? [`Depth  ${frame.waterDepth.toFixed(2)} m`] : []),
      ].join("\n");
    },
  };
}
