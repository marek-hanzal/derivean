import {
  LinearFilter, RepeatWrapping, RGBAFormat, ShaderMaterial, UnsignedByteType, WebGL3DRenderTarget, WebGLRenderTarget,
  type WebGLRenderer,
} from "three";
import { fullscreen, fullscreenVertex } from "./fullscreen";

const ShapeSize = 128;
const DetailSize = 32;
// Shares of the sky the cover levels are tabulated for.
export const CoverSteps = 32;

// Tileable noise for the structure below a weather cell. Shape holds three characters at four features a repeat:
// r heaped (Perlin swollen and then bitten by Worley, which gives domes), g smooth (Perlin), b cellular (Worley);
// a is fine Worley. Detail holds fine Worley for wisps.
const noise = `
  uniform float slice;
  varying vec2 vUv;
  uvec3 pcg(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
    return v;
  }
  vec3 hash(vec3 cell, float period) {
    return vec3(pcg(uvec3(ivec3(mod(cell, period))))) / 4294967295.0;
  }
  float worley(vec3 p, float period) {
    vec3 cell = floor(p * period);
    vec3 inside = fract(p * period);
    float nearest = 1.0;
    for (int z = -1; z <= 1; z++) for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) {
      vec3 offset = vec3(x, y, z);
      vec3 point = hash(cell + offset, period) + offset - inside;
      nearest = min(nearest, dot(point, point));
    }
    return 1.0 - min(sqrt(nearest), 1.0);
  }
  float perlin(vec3 p, float period) {
    vec3 cell = floor(p * period);
    vec3 inside = fract(p * period);
    vec3 ease = inside * inside * inside * (inside * (inside * 6.0 - 15.0) + 10.0);
    float sum = 0.0;
    for (int z = 0; z <= 1; z++) for (int y = 0; y <= 1; y++) for (int x = 0; x <= 1; x++) {
      vec3 corner = vec3(x, y, z);
      vec3 slope = normalize(hash(cell + corner, period) * 2.0 - 1.0);
      vec3 weight = mix(1.0 - ease, ease, corner);
      sum += weight.x * weight.y * weight.z * dot(slope, inside - corner);
    }
    return sum;
  }
  float worleys(vec3 p, float period) {
    return worley(p, period) * 0.625 + worley(p, period * 2.0) * 0.25 + worley(p, period * 4.0) * 0.125;
  }
  float perlins(vec3 p, float period) {
    float sum = perlin(p, period) + perlin(p, period * 2.0) * 0.5 + perlin(p, period * 4.0) * 0.25;
    return clamp(sum * 0.85 + 0.5, 0.0, 1.0);
  }
`;

const shape = `${noise}
  void main() {
    vec3 p = vec3(vUv, slice);
    float soft = perlins(p, 4.0);
    float cells = worleys(p, 4.0);
    float swollen = cells + soft * (1.0 - cells);
    float bite = (1.0 - worleys(p, 8.0)) * 0.5;
    float heaped = clamp((swollen - bite) / (1.0 - bite), 0.0, 1.0);
    gl_FragColor = vec4(heaped, soft, cells, worleys(p, 12.0));
  }
`;

const detail = `${noise}
  void main() {
    vec3 p = vec3(vUv, slice);
    gl_FragColor = vec4(worleys(p, 4.0), worleys(p, 8.0), perlins(p, 4.0), 1.0);
  }
`;

function volume(renderer: WebGLRenderer, size: number, fragmentShader: string) {
  const target = new WebGL3DRenderTarget(size, size, size, { format: RGBAFormat, type: UnsignedByteType, depthBuffer: false });
  const material = new ShaderMaterial({ uniforms: { slice: { value: 0 } }, vertexShader: fullscreenVertex, fragmentShader });
  target.texture.wrapS = target.texture.wrapT = target.texture.wrapR = RepeatWrapping;
  target.texture.minFilter = target.texture.magFilter = LinearFilter;
  for (let layer = 0; layer < size; layer++) {
    material.uniforms.slice.value = (layer + 0.5) / size;
    fullscreen(renderer, material, target, layer);
  }
  material.dispose();
  return target;
}

// For each of the three shape characters, the noise level above which a given share of the volume lies: entry
// i of the result holds the levels for a cover of i / CoverSteps as r, g, b. Measured, so a cover of 0.3 really
// leaves 30 % of a layer in cloud.
function coverLevels(renderer: WebGLRenderer) {
  const size = 96;
  const target = new WebGLRenderTarget(size, size, { format: RGBAFormat, type: UnsignedByteType, depthBuffer: false });
  const material = new ShaderMaterial({ uniforms: { slice: { value: 0 } }, vertexShader: fullscreenVertex, fragmentShader: shape });
  const pixels = new Uint8Array(size * size * 4);
  const samples: number[][] = [[], [], []];
  for (const slice of [0.11, 0.37, 0.63, 0.89]) {
    material.uniforms.slice.value = slice;
    fullscreen(renderer, material, target);
    renderer.readRenderTargetPixels(target, 0, 0, size, size, pixels);
    for (let i = 0; i < pixels.length; i += 4) for (let k = 0; k < 3; k++) samples[k].push(pixels[i + k] / 255);
  }
  material.dispose();
  target.dispose();
  const levels = new Float32Array((CoverSteps + 1) * 4);
  for (let k = 0; k < 3; k++) {
    const sorted = samples[k].sort((a, b) => a - b);
    for (let i = 0; i <= CoverSteps; i++) {
      levels[i * 4 + k] = sorted[Math.min(sorted.length - 1, Math.floor((1 - i / CoverSteps) * sorted.length))];
    }
  }
  return levels;
}

export function noiseVolumes(renderer: WebGLRenderer) {
  const previous = renderer.getRenderTarget();
  const shapes = volume(renderer, ShapeSize, shape);
  const details = volume(renderer, DetailSize, detail);
  const levels = coverLevels(renderer);
  renderer.setRenderTarget(previous);
  return { shapes, details, levels };
}
