import { ShaderMaterial, type IUniform } from "three";
import { fullscreenVertex } from "./fullscreen";

// Blends this frame's noisy march into the picture built up over the frames before. Each pixel looks up where
// its cloud was on screen last frame, taking the cloud to sit where its ray first meets a band; what it finds is
// kept only as far as this frame's neighbourhood allows, which is what stops trails.
export function resolveMaterial(uniforms: Record<string, IUniform>) {
  return new ShaderMaterial({
    uniforms,
    depthTest: false,
    depthWrite: false,
    vertexShader: fullscreenVertex,
    fragmentShader: `
      uniform sampler2D marched;
      uniform sampler2D remembered;
      uniform mat4 cloudCamera;
      uniform mat4 pastClip;
      uniform vec3 eyeMoved;
      uniform vec2 cloudLens;
      uniform vec3 cloudEye;
      uniform vec2 marchTexel;
      uniform float persistence;
      uniform float cloudDatum;
      uniform vec4 cloudRange[3];
      varying vec2 vUv;

      void main() {
        vec4 now = texture(marched, vUv);
        vec3 direction = mat3(cloudCamera) * normalize(vec3((vUv * 2.0 - 1.0) * cloudLens, -1.0));
        float reach = 40000.0;
        for (int band = 0; band < 3; band++) {
          vec4 range = cloudRange[band];
          if (range.z <= 0.0) continue;
          float low = range.x - cloudDatum - cloudEye.y;
          float high = range.y - cloudDatum - cloudEye.y;
          float enter = low > 0.0 ? low / max(direction.y, 0.0001) : high < 0.0 ? high / min(direction.y, -0.0001) : 800.0;
          if (enter > 0.0) reach = min(reach, enter);
        }
        vec4 clip = pastClip * vec4(direction * reach + eyeMoved, 1.0);
        vec2 past = clip.xy / clip.w * 0.5 + 0.5;
        bool found = clip.w > 0.0 && all(greaterThan(past, vec2(0.0))) && all(lessThan(past, vec2(1.0)));

        vec4 least = now;
        vec4 most = now;
        for (int i = 0; i < 8; i++) {
          vec2 offset = vec2(float((i + (i > 3 ? 1 : 0)) % 3 - 1), float((i + (i > 3 ? 1 : 0)) / 3 - 1));
          vec4 beside = texture(marched, vUv + offset * marchTexel);
          least = min(least, beside);
          most = max(most, beside);
        }
        // One frame's march is noisy by design, so the bounds are generous: they are there to stop trails across
        // the sky, not to chase the noise.
        vec4 slack = (most - least) * 0.75 + 0.06;
        vec4 before = clamp(texture(remembered, past), least - slack, most + slack);
        gl_FragColor = mix(now, before, found ? persistence : 0.0);
      }
    `,
  });
}
