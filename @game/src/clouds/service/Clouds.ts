import { DataTexture, FloatType, LinearFilter, RGBAFormat, Vector2, type Vector3, Vector4, type WebGLRenderer } from "three";
import type { Frame } from "../../../contract/GameBridge";
import { BroadRepeat, SmallRepeats, StreakStretch } from "../constant/repeats";
import { Bands } from "../fn/encodeClouds";
import { CoverSteps, noiseVolumes } from "../fn/noiseVolumes";
import type { CloudUniforms } from "../type/CloudUniforms";
import { createCloudField } from "./CloudField";

// Calendar seconds of drift one frame may add. A skipped day is a cut, not a gale.
const LongestGust = 600;
// Radians the wind must turn before a band's streaks are laid along its new direction.
const Veer = 0.2;

const fract = (value: number) => value - Math.floor(value);

// The clouds as the shaders see them: the simulated field in textures, and where the render origin lies in every
// world-anchored noise field. Fine structure rides the band's wind, summed here in doubles; the broad shapes
// ride the low wind in every band, so a storm's tower and anvil travel together. `eye` is the camera's
// render-space position, read as it stands each frame.
export function createClouds(renderer: WebGLRenderer, eye: Vector3) {
  const noise = noiseVolumes(renderer);
  const field = createCloudField();
  const levels = new DataTexture(noise.levels, CoverSteps + 1, 1, RGBAFormat, FloatType);
  levels.minFilter = levels.magFilter = LinearFilter;
  levels.needsUpdate = true;
  const uniforms: CloudUniforms = {
    cloudBands: { value: null },
    cloudShapes: { value: noise.shapes.texture },
    cloudDetails: { value: noise.details.texture },
    cloudLevels: { value: levels },
    cloudGrid: { value: new Vector4(0, 0, 1, 1) },
    cloudDatum: { value: 0 },
    cloudRange: { value: Array.from({ length: Bands }, () => new Vector4()) },
    cloudSteer: { value: new Vector2() },
    cloudDrift: { value: Array.from({ length: Bands }, () => new Vector4()) },
    cloudWind: { value: Array.from({ length: Bands }, () => new Vector2(1, 0)) },
    cloudEye: { value: eye },
  };
  const drifts = Array.from({ length: Bands }, () => new Vector2());
  const headings = [0, 0, 0];
  let calendar: number | undefined;
  let moved = 0;

  return {
    uniforms,
    field,
    // `now` is real milliseconds.
    apply(frame: Frame, now: number) {
      if (frame.clouds) field.ingest(frame.clouds, now);
      moved += calendar === undefined ? 0 : Math.max(-LongestGust, Math.min(LongestGust, frame.calendar.seconds - calendar));
      calendar = frame.calendar.seconds;
    },
    // Places the fields for a render origin and an eye, both in world metres. False while there is no cloud to draw.
    update(origin: Vector3, eye: Vector3, now: number) {
      const latest = field.latest;
      if (!latest) return false;
      field.update(now);
      uniforms.cloudBands.value = field.texture ?? null;
      const span = latest.cell * latest.cells;
      uniforms.cloudGrid.value.set(fract((origin.x - latest.origin[0]) / span), fract((origin.z - latest.origin[1]) / span), 1 / span, latest.cells);
      uniforms.cloudDatum.value = origin.y;

      const ranges = field.ranges(eye.x, eye.z);
      let any = false;
      for (let band = 0; band < Bands; band++) {
        const range = ranges[band];
        uniforms.cloudRange.value[band].set(range.low, range.high, range.active ? 1 : 0, 0);
        any ||= range.active;

        const [windX, windZ] = field.wind(band, eye.x, eye.z);
        drifts[band].x += windX * moved;
        drifts[band].y += windZ * moved;
        const heading = Math.atan2(windZ, windX);
        if (Math.abs(Math.atan2(Math.sin(heading - headings[band]), Math.cos(heading - headings[band]))) > Veer) headings[band] = heading;
        const along = uniforms.cloudWind.value[band].set(Math.cos(headings[band]), Math.sin(headings[band]));
        const repeat = SmallRepeats[band];
        const x = (origin.x - drifts[band].x) / repeat;
        const z = (origin.z - drifts[band].y) / repeat;
        uniforms.cloudDrift.value[band].set(
          fract(x), fract(z), fract((x * along.x + z * along.y) / StreakStretch), fract(-x * along.y + z * along.x),
        );
      }
      moved = 0;
      uniforms.cloudSteer.value.set(fract((origin.x - drifts[0].x) / BroadRepeat), fract((origin.z - drifts[0].y) / BroadRepeat));
      return any;
    },
    dispose() {
      field.dispose();
      levels.dispose();
      noise.shapes.dispose();
      noise.details.dispose();
    },
  };
}

export type Clouds = ReturnType<typeof createClouds>;
