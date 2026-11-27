import {
  Data3DTexture, DataArrayTexture, DataTexture, FloatType, LinearFilter, RepeatWrapping, RGBAFormat, Vector2, Vector3, Vector4,
  type WebGPURenderer,
} from "three/webgpu";
import { texture, texture3D, uniform } from "three/tsl";
import type { Frame } from "../../../contract/GameBridge";
import { BroadRepeat, SmallRepeats, StreakStretch } from "../constant/repeats";
import { Bands, BandLayers } from "../fn/encodeClouds";
import { noiseVolumes } from "../fn/noiseVolumes";
import type { CloudNodes } from "../type/CloudNodes";
import { createCloudField } from "./CloudField";

// Calendar seconds of drift one frame may add. A skipped day is a cut, not a gale.
const LongestGust = 600;
// Radians the wind must turn before a band's streaks are laid along its new direction.
const Veer = 0.2;

const fract = (value: number) => value - Math.floor(value);

// The clouds as the shaders see them: the simulated field in textures, and where the render origin lies in every
// world-anchored noise field. Fine structure rides the band's wind, summed here in doubles; the broad shapes ride
// the low wind in every band, so a storm's tower and anvil travel together. The noise is made on the GPU the first
// time `update` has a WebGPU renderer; until it is back nothing is drawn.
export function createClouds() {
  const field = createCloudField();
  // Stand-ins until the field and the noise are there, filtered as those are, so that a shader built against them
  // binds the same samplers.
  const empty = new DataArrayTexture(new Float32Array(4 * BandLayers * Bands), 1, 1, BandLayers * Bands);
  empty.format = RGBAFormat;
  empty.type = FloatType;
  empty.minFilter = empty.magFilter = LinearFilter;
  empty.wrapS = empty.wrapT = RepeatWrapping;
  empty.needsUpdate = true;
  const flat = new DataTexture(new Float32Array(4), 1, 1, RGBAFormat, FloatType);
  flat.needsUpdate = true;
  const still = new Data3DTexture(new Uint8Array(4), 1, 1, 1);
  still.minFilter = still.magFilter = LinearFilter;
  still.wrapS = still.wrapT = still.wrapR = RepeatWrapping;
  still.needsUpdate = true;
  const nodes: CloudNodes = {
    grid: uniform(new Vector4(0, 0, 1, 1)),
    steer: uniform(new Vector2()),
    datum: uniform(0),
    ranges: Array.from({ length: Bands }, () => uniform(new Vector4())),
    drifts: Array.from({ length: Bands }, () => uniform(new Vector4())),
    wind01: uniform(new Vector4(1, 0, 1, 0)),
    wind2: uniform(new Vector2(1, 0)),
    eye: uniform(new Vector3()),
    bands: texture(empty),
    shapes: texture3D(still),
    details: texture3D(still),
    levels: texture(flat),
  };
  const drifts = Array.from({ length: Bands }, () => new Vector2());
  const headings = [0, 0, 0];
  const along = new Vector2();
  let calendar: number | undefined;
  let moved = 0;
  let noise: ReturnType<typeof noiseVolumes> | undefined;
  let ready = false;
  let disposed = false;

  return {
    nodes,
    field,
    // `now` is real milliseconds.
    apply(frame: Frame, now: number) {
      if (frame.clouds) field.ingest(frame.clouds, now);
      moved += calendar === undefined ? 0 : Math.max(-LongestGust, Math.min(LongestGust, frame.calendar.seconds - calendar));
      calendar = frame.calendar.seconds;
    },
    // Places the fields for a render origin and an eye, both in world metres. False while there is no cloud to
    // draw.
    update(renderer: WebGPURenderer, origin: Vector3, eye: Vector3, now: number) {
      if (!noise) {
        noise = noiseVolumes(renderer);
        nodes.shapes.value = noise.shapes;
        nodes.details.value = noise.details;
        noise.levels.then((levels) => {
          if (disposed) {
            levels.dispose();
            return;
          }
          nodes.levels.value = levels;
          ready = true;
        }).catch((error: unknown) => console.error("The clouds' noise could not be measured; no clouds are drawn.", error));
      }
      const latest = field.latest;
      nodes.eye.value.copy(eye).sub(origin);
      if (!latest || !ready) return false;
      field.update(now);
      if (field.texture && nodes.bands.value !== field.texture) nodes.bands.value = field.texture;
      const span = latest.cell * latest.cells;
      nodes.grid.value.set(fract((origin.x - latest.origin[0]) / span), fract((origin.z - latest.origin[1]) / span), 1 / span, latest.cells);
      nodes.datum.value = origin.y;

      const ranges = field.ranges(eye.x, eye.z);
      let any = false;
      for (let band = 0; band < Bands; band++) {
        const range = ranges[band];
        nodes.ranges[band].value.set(range.low, range.high, range.active ? 1 : 0, 0);
        any ||= range.active;

        const [windX, windZ] = field.wind(band, eye.x, eye.z);
        drifts[band].x += windX * moved;
        drifts[band].y += windZ * moved;
        const heading = Math.atan2(windZ, windX);
        if (Math.abs(Math.atan2(Math.sin(heading - headings[band]), Math.cos(heading - headings[band]))) > Veer) headings[band] = heading;
        along.set(Math.cos(headings[band]), Math.sin(headings[band]));
        if (band === 0) nodes.wind01.value.setX(along.x).setY(along.y);
        else if (band === 1) nodes.wind01.value.setZ(along.x).setW(along.y);
        else nodes.wind2.value.copy(along);
        const repeat = SmallRepeats[band];
        const x = (origin.x - drifts[band].x) / repeat;
        const z = (origin.z - drifts[band].y) / repeat;
        nodes.drifts[band].value.set(fract(x), fract(z), fract((x * along.x + z * along.y) / StreakStretch), fract(-x * along.y + z * along.x));
      }
      moved = 0;
      nodes.steer.value.set(fract((origin.x - drifts[0].x) / BroadRepeat), fract((origin.z - drifts[0].y) / BroadRepeat));
      return any;
    },
    dispose() {
      disposed = true;
      field.dispose();
      empty.dispose();
      flat.dispose();
      still.dispose();
      noise?.shapes.dispose();
      noise?.details.dispose();
      if (ready) (nodes.levels.value as DataTexture).dispose();
    },
  };
}

export type Clouds = ReturnType<typeof createClouds>;
