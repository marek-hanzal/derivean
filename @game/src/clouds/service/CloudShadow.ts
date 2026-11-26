import {
  LinearFilter, RGBAFormat, ShaderMaterial, UnsignedByteType, Vector3, Vector4, type WebGLRenderer, WebGLRenderTarget,
} from "three";
import { cloudDensity } from "../constant/density";
import { ShadowMs, ShadowSize, ShadowSpan } from "../constant/shadow";
import { fullscreen, fullscreenVertex } from "../fn/fullscreen";
import type { CloudShade } from "../type/CloudShade";
import type { CloudUniforms } from "../type/CloudUniforms";

const Texel = ShadowSpan / ShadowSize;

// The shadows the clouds cast, as a map of sunlight reaching a level plane around the eye. Each texel looks up
// the sun's ray through the three bands of the same density field the clouds are drawn from, so the shadows are
// the clouds' own and move with them. The map is anchored to the world on whole texels and redrawn a few times
// a second.
export function createCloudShadow(clouds: CloudUniforms, sun: { value: Vector3 }) {
  const target = new WebGLRenderTarget(ShadowSize, ShadowSize, { format: RGBAFormat, type: UnsignedByteType, depthBuffer: false });
  target.texture.minFilter = target.texture.magFilter = LinearFilter;
  const place = { value: new Vector4() };
  const uniforms: CloudShade = { cloudShadowMap: { value: target.texture }, cloudShadowPlace: { value: new Vector4() } };
  const material = new ShaderMaterial({
    uniforms: { ...clouds, skySun: sun, shadowPlace: place },
    depthTest: false,
    depthWrite: false,
    vertexShader: fullscreenVertex,
    fragmentShader: `
      uniform vec4 shadowPlace;
      uniform vec3 skySun;
      varying vec2 vUv;
      ${cloudDensity}
      void main() {
        vec3 foot = vec3(shadowPlace.x + (vUv.x - 0.5) * shadowPlace.z, shadowPlace.w, shadowPlace.y + (vUv.y - 0.5) * shadowPlace.z);
        float slant = 1.0 / max(skySun.y, 0.12);
        float depth = 0.0;
        for (int band = 0; band < 3; band++) {
          vec4 range = cloudRange[band];
          if (range.z <= 0.0) continue;
          // The layer's own base and top where the ray crosses the middle of the band.
          vec3 middle = foot + skySun * ((0.5 * (range.x + range.y) - cloudDatum - foot.y) * slant);
          vec4 shape = texture(cloudBands, vec3(cloudCell(middle), float(band * 2)));
          if (shape.x < 0.01) continue;
          for (int i = 0; i < 3; i++) {
            float height = mix(shape.y, shape.z, (float(i) + 0.5) / 3.0) - cloudDatum;
            vec3 p = foot + skySun * ((height - foot.y) * slant);
            float ice;
            depth += bandCloud(band, p, cloudCell(p), false, ice) * (shape.z - shape.y) / 3.0 * slant;
          }
        }
        // The straight beam dies off as Beer and Lambert say; light scattered forward keeps thin cloud bright, and
        // some always comes out of the base of the thickest.
        gl_FragColor = vec4(0.12 + 0.88 * mix(exp(-depth), exp(-depth * 0.25), 0.3), 0.0, 0.0, 1.0);
      }
    `,
  });
  const centre = new Vector3();
  let drawn = -Infinity;

  return {
    uniforms,
    // Redraws the map when it is due. `origin` and `eye` are world metres, `ground` the height to lay the plane at.
    update(renderer: WebGLRenderer, origin: Vector3, eye: Vector3, ground: number, lit: boolean, now: number) {
      if (!lit) {
        uniforms.cloudShadowPlace.value.setScalar(0);
        return;
      }
      if (now - drawn >= ShadowMs) {
        drawn = now;
        centre.set(Math.round(eye.x / Texel) * Texel, Math.round(ground / 50) * 50, Math.round(eye.z / Texel) * Texel);
        place.value.set(centre.x - origin.x, centre.z - origin.z, ShadowSpan, centre.y - origin.y);
        const previous = renderer.getRenderTarget();
        fullscreen(renderer, material, target);
        renderer.setRenderTarget(previous);
      }
      // The map stays where it was drawn in the world while the render origin moves under it.
      uniforms.cloudShadowPlace.value.set(centre.x - origin.x, centre.z - origin.z, ShadowSpan, centre.y - origin.y);
    },
    dispose() {
      target.dispose();
      material.dispose();
    },
  };
}
