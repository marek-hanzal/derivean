import {
  LinearFilter, type Node, NodeMaterial, QuadMesh, RedFormat, RenderTarget, UnsignedByteType, Vector3, Vector4, type WebGPURenderer,
} from "three/webgpu";
import { uniform, uv, vec4, wgsl, wgslFn } from "three/tsl";
import { DensityWgsl } from "../constant/density";
import { ForwardShare, ForwardThinning, LeastThrough, ShadowMs, ShadowSize, ShadowSpans } from "../constant/shadow";
import { CloudParams, CloudSkyOf, CloudTextures, cloudArgs } from "../fn/cloudArgs";
import { sunThrough } from "../fn/sunThrough";
import type { CloudNodes } from "../type/CloudNodes";
import type { CloudShade } from "../type/CloudShade";

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

// The shadows the clouds cast, as maps of sunlight reaching a level plane around the eye, one near and one far
// (ShadowSpans), so the shadows are the clouds' own and move with them. Each map is anchored to the world on whole
// texels of its own and redrawn a few times a second.
export function createCloudShadow(clouds: CloudNodes, sun: Node<"vec3">) {
  const args = cloudArgs(clouds);
  const maps = ShadowSpans.map((span) => {
    // One byte a texel: the share let through is all a map holds (r8unorm).
    const target = new RenderTarget(ShadowSize, ShadowSize, { format: RedFormat, type: UnsignedByteType, depthBuffer: false });
    target.texture.minFilter = target.texture.magFilter = LinearFilter;
    const drawn = uniform(new Vector4());
    const material = new NodeMaterial();
    material.fragmentNode = vec4(throughNode({ at: uv(), place: drawn, sun, ...args }) as Node<"float">, 0, 0, 1);
    return {
      span, texel: span / ShadowSize, target, drawn, material, quad: new QuadMesh(material), place: uniform(new Vector4()), centre: new Vector3(),
      last: -Infinity,
    };
  });
  const shade: CloudShade = {
    maps: [maps[0].target.texture, maps[1].target.texture], places: [maps[0].place, maps[1].place], sun, beyond: uniform(1),
  };
  return {
    shade,
    // Redraws the maps when they are due. `origin` and `eye` are world metres, `ground` the height to lay the plane
    // at; `overhead` is the share of the sky the cloud over the eye covers and its optical depth.
    update(renderer: WebGPURenderer, origin: Vector3, eye: Vector3, ground: number, lit: boolean, now: number,
      overhead: readonly [number, number]) {
      if (!lit) {
        for (const map of maps) {
          map.place.value.setScalar(0);
          map.last = -Infinity;
        }
        shade.beyond.value = 1;
        return;
      }
      shade.beyond.value = 1 - overhead[0] * (1 - sunThrough(overhead[1]));
      // A map that has never been drawn is drawn at once; otherwise at most one map a frame, the one longest due,
      // so two draws never fall in one frame and neither map waits on the other however slow the frames are.
      const due = maps.filter((map) => now - map.last >= ShadowMs);
      const next = due.reduce<(typeof maps)[number] | undefined>((oldest, map) => (oldest && oldest.last <= map.last ? oldest : map), undefined);
      for (const map of maps) {
        const { span, texel, target, drawn, quad, place, centre } = map;
        if (map === next || map.last === -Infinity) {
          map.last = now;
          centre.set(Math.round(eye.x / texel) * texel, Math.round(ground / 50) * 50, Math.round(eye.z / texel) * texel);
          drawn.value.set(centre.x - origin.x, centre.z - origin.z, span, centre.y - origin.y);
          const previous = renderer.getRenderTarget();
          renderer.setRenderTarget(target);
          quad.render(renderer);
          renderer.setRenderTarget(previous);
        }
        // The map stays where it was drawn in the world while the render origin moves under it.
        place.value.set(centre.x - origin.x, centre.z - origin.z, span, centre.y - origin.y);
      }
    },
    dispose() {
      for (const { target, material } of maps) {
        target.dispose();
        material.dispose();
      }
    },
  };
}
