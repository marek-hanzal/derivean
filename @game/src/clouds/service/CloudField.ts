import { DataArrayTexture, FloatType, LinearFilter, RepeatWrapping, RGBAFormat } from "three/webgpu";
import type { CloudField } from "../../../contract/GameBridge";
import type { HazeAir } from "../../sky/type/HazeAir";
import { BandLayers, Bands, encodeClouds } from "../fn/encodeClouds";

// Cells around the eye whose clouds bound what a ray can meet before the haze takes it.
const Reach = 3;
const UploadMs = 100;

export interface CloudRange { low: number; high: number; active: boolean }

// The engine's cloud snapshots as textures. A new snapshot is not shown at once: what is on screen blends into it
// over about the time snapshots take to arrive, so the sky never jumps.
export function createCloudField() {
  let latest: CloudField | undefined;
  let texture: DataArrayTexture | undefined;
  let from: Float32Array | undefined;
  let to: Float32Array | undefined;
  let arrived = 0;
  let span = 1000;
  let uploaded = 0;
  let share = 1;

  function blend() {
    if (!texture || !from || !to) return;
    const shown = texture.image.data as Float32Array;
    for (let i = 0; i < shown.length; i++) shown[i] = from[i] + (to[i] - from[i]) * share;
    texture.needsUpdate = true;
  }

  return {
    get latest() { return latest; },
    get texture() { return texture; },
    // `now` is real milliseconds.
    ingest(field: CloudField, now: number) {
      const layers = encodeClouds(field);
      if (!texture || texture.image.width !== field.cells) {
        texture?.dispose();
        texture = new DataArrayTexture(new Float32Array(layers), field.cells, field.cells, BandLayers * Bands);
        texture.format = RGBAFormat;
        texture.type = FloatType;
        texture.minFilter = texture.magFilter = LinearFilter;
        texture.wrapS = texture.wrapT = RepeatWrapping;
        texture.needsUpdate = true;
        from = to = layers;
        share = 1;
      } else {
        // What is shown now becomes the start of the next blend, wherever the last one had got to.
        from = new Float32Array(texture.image.data as Float32Array);
        to = layers;
        span = Math.max(250, Math.min(4000, now - arrived));
        share = 0;
      }
      arrived = now;
      latest = field;
    },
    update(now: number) {
      if (share >= 1 || now - uploaded < UploadMs) return;
      uploaded = now;
      share = Math.min(1, (now - arrived) / span);
      blend();
    },
    // Heights between which each band holds cloud near a world position, from the snapshot being blended to.
    ranges(x: number, z: number): CloudRange[] {
      if (!latest || !to || !from) return [];
      const { cells, cell, origin } = latest;
      const cx = Math.floor((x - origin[0]) / cell);
      const cz = Math.floor((z - origin[1]) / cell);
      return Array.from({ length: Bands }, (_, band) => {
        const layer = band * BandLayers * cells * cells * 4;
        const range = { low: Infinity, high: -Infinity, active: false };
        for (let dz = -Reach; dz <= Reach; dz++) {
          for (let dx = -Reach; dx <= Reach; dx++) {
            const i = layer + ((((cz + dz) % cells) + cells) % cells * cells + (((cx + dx) % cells) + cells) % cells) * 4;
            for (const source of [from!, to!]) {
              if (source[i] < 0.01) continue;
              range.low = Math.min(range.low, source[i + 1]);
              range.high = Math.max(range.high, source[i + 2]);
              range.active = true;
            }
          }
        }
        return range;
      });
    },
    // Wind of a band over a world position in m/s, to carry its fine structure along.
    wind(band: number, x: number, z: number): [number, number] {
      if (!latest) return [0, 0];
      const { cells, cell, origin } = latest;
      const cx = ((Math.floor((x - origin[0]) / cell) % cells) + cells) % cells;
      const cz = ((Math.floor((z - origin[1]) / cell) % cells) + cells) % cells;
      return [latest.bands[band].windX[cz * cells + cx], latest.bands[band].windZ[cz * cells + cx]];
    },
    // The haze of the weather over a world position, linear between cell centres: optical depth of the low band's
    // haze, the height of its top above the sea and its thickness, and the veil above it. The haze's colour is
    // weighed by each cell's haze.
    air(x: number, z: number): HazeAir | undefined {
      if (!latest) return undefined;
      const { cells, cell, origin, hazeDepth, hazeTop, hazeColour, aerosolDepth } = latest;
      const fx = (x - origin[0]) / cell - 0.5, fz = (z - origin[1]) / cell - 0.5;
      const x0 = Math.floor(fx), z0 = Math.floor(fz), tx = fx - x0, tz = fz - z0;
      const corners = [[0, 0, (1 - tx) * (1 - tz)], [1, 0, tx * (1 - tz)], [0, 1, (1 - tx) * tz], [1, 1, tx * tz]] as const;
      const index = (dx: number, dz: number) => (((z0 + dz) % cells + cells) % cells) * cells + ((x0 + dx) % cells + cells) % cells;
      const at = (values: Float32Array) => corners.reduce((sum, [dx, dz, weight]) => sum + values[index(dx, dz)] * weight, 0);
      // Entries per cell of hazeColour (the contract's five).
      const stride = hazeColour.length / hazeDepth.length;
      // A ratio over the four cells, each weighed by its own denominator: the depth at 550 nm for the spectrum,
      // the depth at that colour for the albedo.
      const ratio = (entry: number, over: (at: number) => number) => {
        let sum = 0, weights = 0;
        for (const [dx, dz, weight] of corners) {
          const share = weight * Math.max(over(index(dx, dz)), 1e-6);
          sum += hazeColour[index(dx, dz) * stride + entry] * share;
          weights += share;
        }
        return sum / weights;
      };
      const depthAt = (cell: number) => hazeDepth[cell];
      const colourAt = (colour: number) => (cell: number) => hazeDepth[cell] * (colour === 1 ? 1 : hazeColour[cell * stride + (colour === 0 ? 0 : 1)]);
      return {
        haze: at(hazeDepth), top: at(hazeTop), thickness: latest.hazeThickness, veil: at(aerosolDepth),
        spectrum: [ratio(0, depthAt), 1, ratio(1, depthAt)],
        albedo: [ratio(2, colourAt(0)), ratio(3, colourAt(1)), ratio(4, colourAt(2))],
      };
    },
    // The cloud over a world position as the ground's light sees it: the share of the sky covered, and the optical
    // depth of what covers it.
    overcast(x: number, z: number): [number, number] {
      if (!latest) return [0, 0];
      const { cells, cell, origin } = latest;
      const cx = ((Math.floor((x - origin[0]) / cell) % cells) + cells) % cells;
      const cz = ((Math.floor((z - origin[1]) / cell) % cells) + cells) % cells;
      let clear = 1;
      let depth = 0;
      for (const band of latest.bands) {
        clear *= 1 - Math.min(1, band.cover[cz * cells + cx]);
        depth += band.depth[cz * cells + cx];
      }
      const cover = 1 - clear;
      return [cover, cover > 0 ? depth / cover : 0];
    },
    dispose() { texture?.dispose(); },
  };
}

export type CloudFieldTextures = ReturnType<typeof createCloudField>;
