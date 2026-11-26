import { LinearFilter, type Node, NodeMaterial, QuadMesh, RenderTarget, UnsignedByteType, Vector3, Vector4, type WebGPURenderer } from "three/webgpu";
import { uniform, uv, vec4, wgsl, wgslFn } from "three/tsl";
import { DensityWgsl } from "../constant/density";
import { ForwardShare, ForwardThinning, LeastThrough, ShadowMs, ShadowSize, ShadowSpan } from "../constant/shadow";
import { CloudParams, CloudSkyOf, CloudTextures, cloudArgs } from "../fn/cloudArgs";
import { sunThrough } from "../fn/sunThrough";
import type { CloudNodes } from "../type/CloudNodes";
import type { CloudShade } from "../type/CloudShade";

const Texel = ShadowSpan / ShadowSize;
const literal = (value: number) => value.toFixed(4);

// Each texel looks up the sun's ray through the three bands of the same density field the clouds are drawn from,
// and lets through what sunThrough says.
const throughNode = wgslFn(`
fn cloudThrough(at: vec2f, place: vec4f, sun: vec3f, ${CloudParams}) -> f32 {
  let c = ${CloudSkyOf};
  let foot = vec3f(place.x + (at.x - 0.5) * place.z, place.w, place.y + (at.y - 0.5) * place.z);
  let slant = 1.0 / max(sun.y, 0.12);
  var depth = 0.0;
  for (var band = 0; band < 3; band++) {
    let range = cloudRange(c, band);
    if (range.z <= 0.0) { continue; }
    // The layer's own base and top where the ray crosses the middle of the band.
    let middle = foot + sun * ((0.5 * (range.x + range.y) - c.datum - foot.y) * slant);
    let shape = textureSampleLevel(bands, bandSampler, cloudCell(c, middle), band * 2, 0.0);
    if (shape.x < 0.01) { continue; }
    for (var i = 0; i < 3; i++) {
      let height = mix(shape.y, shape.z, (f32(i) + 0.5) / 3.0) - c.datum;
      let p = foot + sun * ((height - foot.y) * slant);
      depth += bandCloud(c, ${CloudTextures}, band, p, cloudCell(c, p), false).x * (shape.z - shape.y) / 3.0 * slant;
    }
  }
  return ${literal(LeastThrough)} + ${literal(1 - LeastThrough)} * mix(exp(-depth), exp(-depth * ${literal(ForwardThinning)}), ${literal(ForwardShare)});
}
`, [wgsl(DensityWgsl)]);

// The shadows the clouds cast, as a map of sunlight reaching a level plane around the eye, so the shadows are the
// clouds' own and move with them. The map is anchored to the world on whole texels and redrawn a few times a
// second.
export function createCloudShadow(clouds: CloudNodes, sun: Node<"vec3">) {
  const target = new RenderTarget(ShadowSize, ShadowSize, { type: UnsignedByteType, depthBuffer: false });
  target.texture.minFilter = target.texture.magFilter = LinearFilter;
  const drawn = uniform(new Vector4());
  const shade: CloudShade = { map: target.texture, place: uniform(new Vector4()), sun, beyond: uniform(1) };
  const material = new NodeMaterial();
  material.fragmentNode = vec4(throughNode({ at: uv(), place: drawn, sun, ...cloudArgs(clouds) }) as Node<"float">, 0, 0, 1);
  const quad = new QuadMesh(material);
  const centre = new Vector3();
  let last = -Infinity;

  return {
    shade,
    // Redraws the map when it is due. `origin` and `eye` are world metres, `ground` the height to lay the plane at;
    // `overhead` is the share of the sky the cloud over the eye covers and its optical depth.
    update(renderer: WebGPURenderer, origin: Vector3, eye: Vector3, ground: number, lit: boolean, now: number,
      overhead: readonly [number, number]) {
      if (!lit) {
        shade.place.value.setScalar(0);
        shade.beyond.value = 1;
        last = -Infinity;
        return;
      }
      shade.beyond.value = 1 - overhead[0] * (1 - sunThrough(overhead[1]));
      if (now - last >= ShadowMs) {
        last = now;
        centre.set(Math.round(eye.x / Texel) * Texel, Math.round(ground / 50) * 50, Math.round(eye.z / Texel) * Texel);
        drawn.value.set(centre.x - origin.x, centre.z - origin.z, ShadowSpan, centre.y - origin.y);
        const previous = renderer.getRenderTarget();
        renderer.setRenderTarget(target);
        quad.render(renderer);
        renderer.setRenderTarget(previous);
      }
      // The map stays where it was drawn in the world while the render origin moves under it.
      shade.place.value.set(centre.x - origin.x, centre.z - origin.z, ShadowSpan, centre.y - origin.y);
    },
    dispose() {
      target.dispose();
      material.dispose();
    },
  };
}
