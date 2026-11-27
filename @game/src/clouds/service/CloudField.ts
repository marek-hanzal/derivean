import { int, screenCoordinate, texture, uniform, wgslFn } from "three/tsl";
import {
  DataArrayTexture, FloatType, LinearFilter, NodeMaterial, QuadMesh, RenderTarget, RepeatWrapping, RGBAFormat, type Texture,
  type WebGPURenderer,
} from "three/webgpu";
import type { CloudField } from "../../../contract/GameBridge";
import type { HazeAir } from "../../sky/type/HazeAir";
import { BandLayers, Bands, encodeClouds } from "../fn/encodeClouds";
import { weatherHaze } from "../fn/weatherHaze";

// Cells around the eye whose clouds bound what a ray can meet before the haze takes it.
const Reach = 3;
// Milliseconds between blends drawn.
const BlendMs = 100;

export interface CloudRange { low: number; high: number; active: boolean }

// One texel of one layer of the blend between two snapshots, from + (to − from) · share.
const blendNode = wgslFn(`
fn bandsBetween(older: texture_2d_array<f32>, newer: texture_2d_array<f32>, at: vec2f, layer: i32, share: f32) -> vec4f {
  let texel = vec2i(at);
  let start = textureLoad(older, texel, layer, 0);
  return start + (textureLoad(newer, texel, layer, 0) - start) * share;
}
`);

const layered = <Made extends Texture>(texture: Made) => {
  texture.format = RGBAFormat;
  texture.type = FloatType;
  texture.minFilter = texture.magFilter = LinearFilter;
  texture.wrapS = texture.wrapT = RepeatWrapping;
  texture.generateMipmaps = false;
  return texture;
};

// The engine's cloud snapshots as textures. A new snapshot is not shown at once: what is on screen blends into it
// over about the time snapshots take to arrive, so the sky never jumps. A snapshot goes up once, as it arrives; the
// blend is drawn on the GPU from what was shown when it arrived and the new snapshot, into a texture the clouds
// read, at most every BlendMs. At the end of a blend the snapshot itself is shown. The GPU holds two snapshots and
// two blends, four times the one field; the CPU keeps the start of the blend and the snapshot for `ranges`.
export function createCloudField() {
  let latest: CloudField | undefined;
  let cells = 0;
  // Two snapshots and two blends, so the one a blend starts from is never the one written.
  let uploads: DataArrayTexture[] = [];
  let blends: RenderTarget[] = [];
  let start: Texture | undefined;
  let end: Texture | undefined;
  let shown: Texture | undefined;
  // What was shown when the snapshot arrived and the snapshot, on the CPU, for the heights a band holds cloud
  // between.
  let from: Float32Array | undefined;
  let to: Float32Array | undefined;
  let held: Float32Array | undefined;
  let arrived = 0;
  let span = 1000;
  let blended = 0;
  let share = 1;
  // Bound in place of a snapshot until the first blend is drawn; never drawn from.
  const none = layered(new DataArrayTexture(new Float32Array(4 * BandLayers * Bands), 1, 1, BandLayers * Bands));
  const fromNode = texture(none);
  const toNode = texture(none);
  const shareNode = uniform(1);
  const materials = Array.from({ length: BandLayers * Bands }, (_, layer) => {
    const material = new NodeMaterial();
    material.fragmentNode = blendNode({ older: fromNode, newer: toNode, at: screenCoordinate.xy, layer: int(layer), share: shareNode });
    return material;
  });
  const quads = materials.map((material) => new QuadMesh(material));

  function resize(size: number) {
    for (const upload of uploads) upload.dispose();
    for (const blend of blends) blend.dispose();
    cells = size;
    // Each takes a snapshot's own layers as it goes up.
    uploads = [0, 1].map(() => layered(new DataArrayTexture(null, size, size, BandLayers * Bands)));
    blends = [0, 1].map(() => {
      const target = new RenderTarget(size, size, { depth: BandLayers * Bands, depthBuffer: false, type: FloatType, format: RGBAFormat });
      layered(target.texture);
      return target;
    });
    held = new Float32Array(size * size * 4 * BandLayers * Bands);
  }

  function draw(renderer: WebGPURenderer) {
    const target = blends[0].texture === start ? blends[1] : blends[0];
    fromNode.value = start!;
    toNode.value = end!;
    shareNode.value = share;
    const previous = renderer.getRenderTarget();
    const [face, level] = [renderer.getActiveCubeFace(), renderer.getActiveMipmapLevel()];
    for (let layer = 0; layer < quads.length; layer++) {
      renderer.setRenderTarget(target, layer);
      quads[layer].render(renderer);
    }
    renderer.setRenderTarget(previous, face, level);
    shown = target.texture;
  }

  const ranges = Array.from({ length: Bands }, (): CloudRange => ({ low: Infinity, high: -Infinity, active: false }));
  const ranged = { x: NaN, z: NaN, of: undefined as CloudField | undefined };
  const air: HazeAir = { haze: 0, top: 0, thickness: 0, veil: 0, spectrum: [0, 1, 0], albedo: [0, 0, 0] };

  return {
    get latest() { return latest; },
    get texture() { return shown; },
    // `now` is real milliseconds.
    ingest(field: CloudField, now: number) {
      const layers = encodeClouds(field);
      if (!shown || field.cells !== cells) {
        resize(field.cells);
        uploads[0].image.data = layers;
        uploads[0].needsUpdate = true;
        start = end = shown = uploads[0];
        from = to = layers;
        share = 1;
      } else {
        // What is shown now becomes the start of the next blend, wherever the last one had got to: the snapshot at the
        // end of a blend, the blend drawn last part of the way, or the start while nothing has been drawn.
        if (shown === end) from = to;
        else if (shown !== start) {
          for (let i = 0; i < held!.length; i++) held![i] = from![i] + (to![i] - from![i]) * share;
          from = held;
        }
        start = shown;
        const upload = uploads[0] === start ? uploads[1] : uploads[0];
        upload.image.data = layers;
        upload.needsUpdate = true;
        end = upload;
        to = layers;
        span = Math.max(250, Math.min(4000, now - arrived));
        share = 0;
      }
      arrived = now;
      latest = field;
    },
    // Draws the blend into a target of its own, so it is called outside any render.
    update(renderer: WebGPURenderer, now: number) {
      if (share >= 1 || now - blended < BlendMs) return;
      blended = now;
      share = Math.min(1, (now - arrived) / span);
      if (share >= 1) shown = end;
      else draw(renderer);
    },
    // Heights between which each band holds cloud near a world position, from what was shown when the snapshot
    // arrived and the snapshot. They change only with the cell the position is in and the snapshot, so they are
    // kept until either does; the ranges returned are this field's own, valid until the next call.
    ranges(x: number, z: number): readonly CloudRange[] {
      if (!latest || !to || !from) return [];
      const { cell, origin } = latest;
      const cx = Math.floor((x - origin[0]) / cell);
      const cz = Math.floor((z - origin[1]) / cell);
      if (cx === ranged.x && cz === ranged.z && latest === ranged.of) return ranges;
      [ranged.x, ranged.z, ranged.of] = [cx, cz, latest];
      for (let band = 0; band < Bands; band++) {
        const layer = band * BandLayers * cells * cells * 4;
        const range = ranges[band];
        range.low = Infinity;
        range.high = -Infinity;
        range.active = false;
        for (let dz = -Reach; dz <= Reach; dz++) {
          for (let dx = -Reach; dx <= Reach; dx++) {
            const i = layer + ((((cz + dz) % cells) + cells) % cells * cells + (((cx + dx) % cells) + cells) % cells) * 4;
            for (let source = 0; source < 2; source++) {
              const values = source ? to : from;
              if (values[i] < 0.01) continue;
              range.low = Math.min(range.low, values[i + 1]);
              range.high = Math.max(range.high, values[i + 2]);
              range.active = true;
            }
          }
        }
      }
      return ranges;
    },
    // Wind of a band over a world position in m/s, to carry its fine structure along.
    wind(band: number, x: number, z: number): [number, number] {
      if (!latest) return [0, 0];
      const { cells, cell, origin } = latest;
      const cx = ((Math.floor((x - origin[0]) / cell) % cells) + cells) % cells;
      const cz = ((Math.floor((z - origin[1]) / cell) % cells) + cells) % cells;
      return [latest.bands[band].windX[cz * cells + cx], latest.bands[band].windZ[cz * cells + cx]];
    },
    // The haze of the weather over a world position (weatherHaze); the haze returned is this field's own, valid
    // until the next call.
    air(x: number, z: number): HazeAir | undefined {
      return latest && weatherHaze(latest, x, z, air);
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
    dispose() {
      for (const upload of uploads) upload.dispose();
      for (const blend of blends) blend.dispose();
      none.dispose();
      for (const material of materials) material.dispose();
    },
  };
}

export type CloudFieldTextures = ReturnType<typeof createCloudField>;
