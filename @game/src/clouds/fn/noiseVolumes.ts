import {
  DataTexture, LinearFilter, RepeatWrapping, RGBAFormat, Storage3DTexture, UnsignedByteType, type WebGPURenderer,
} from "three/webgpu";
import { Fn, instanceIndex, instancedArray, textureStore, uvec3, vec3, wgsl, wgslFn } from "three/tsl";
import { CoverSteps } from "../constant/density";
import { NoiseWgsl } from "../constant/noise";

const ShapeSize = 128;
const DetailSize = 32;
// Texels a side of the slices the cover levels are measured on, how many there are, and where the first lies and
// how far apart they are through the volume.
const LevelSize = 96;
const LevelSlices = 4;
const FirstSlice = 0.11;
const SliceStep = 0.26;

const noise = wgsl(NoiseWgsl);
const shapeAt = wgslFn("fn shapeAt(p: vec3f) -> vec4f { return shapeNoise(p); }", [noise]);
const detailAt = wgslFn("fn detailAt(p: vec3f) -> vec4f { return detailNoise(p); }", [noise]);

// The noise in a repeating 3D texture, one compute invocation a texel.
function volume(renderer: WebGPURenderer, size: number, at: typeof shapeAt) {
  const target = new Storage3DTexture(size, size, size);
  target.format = RGBAFormat;
  target.type = UnsignedByteType;
  target.wrapS = target.wrapT = target.wrapR = RepeatWrapping;
  target.minFilter = target.magFilter = LinearFilter;
  const fill = Fn(() => {
    const x = instanceIndex.mod(size);
    const y = instanceIndex.div(size).mod(size);
    const z = instanceIndex.div(size * size);
    textureStore(target, uvec3(x, y, z), at(vec3(x, y, z).add(0.5).div(size)));
  })().compute(size ** 3);
  renderer.compute(fill);
  fill.dispose();
  return target;
}

// For each of the three shape characters, the noise level above which a given share of the volume lies: texel i
// holds the levels for a cover of i / CoverSteps as r, g, b. Measured on slices of the same noise, rounded as the
// volume's bytes round it, so a cover of 0.3 really leaves 30 % of a layer in cloud, and kept in bytes, which hold
// those levels exactly and which the GPU filters between covers. The GPU answers later.
async function coverLevels(renderer: WebGPURenderer) {
  const each = LevelSize * LevelSize;
  const samples = instancedArray(each * LevelSlices, "vec4");
  const measure = Fn(() => {
    const x = instanceIndex.mod(LevelSize);
    const y = instanceIndex.div(LevelSize).mod(LevelSize);
    const slice = instanceIndex.div(each);
    const depth = slice.toFloat().mul(SliceStep).add(FirstSlice);
    samples.element(instanceIndex).assign(shapeAt(vec3(x.toFloat().add(0.5).div(LevelSize), y.toFloat().add(0.5).div(LevelSize), depth)));
  })().compute(each * LevelSlices);
  renderer.compute(measure);
  const values = new Float32Array(await renderer.getArrayBufferAsync(samples.value));
  measure.dispose();
  samples.value.dispose?.();
  const levels = new Uint8Array((CoverSteps + 1) * 4);
  for (let k = 0; k < 3; k++) {
    const sorted = Uint8Array.from({ length: values.length / 4 }, (_, i) => Math.round(Math.min(1, Math.max(0, values[i * 4 + k])) * 255)).sort();
    for (let i = 0; i <= CoverSteps; i++) levels[i * 4 + k] = sorted[Math.min(sorted.length - 1, Math.floor((1 - i / CoverSteps) * sorted.length))];
  }
  const texture = new DataTexture(levels, CoverSteps + 1, 1, RGBAFormat, UnsignedByteType);
  texture.minFilter = texture.magFilter = LinearFilter;
  texture.needsUpdate = true;
  return texture;
}

// The noise the clouds' small structure is drawn from, made once on the GPU. `levels` resolves once the measured
// cover levels are back.
export function noiseVolumes(renderer: WebGPURenderer) {
  return {
    shapes: volume(renderer, ShapeSize, shapeAt),
    details: volume(renderer, DetailSize, detailAt),
    levels: coverLevels(renderer),
  };
}
