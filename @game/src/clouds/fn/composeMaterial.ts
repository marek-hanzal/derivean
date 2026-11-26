import { ShaderMaterial, type IUniform } from "three";
import { fullscreenVertex } from "./fullscreen";

// Lays the low-resolution clouds over the scene. Each pixel weighs the four cloud texels around it by how close
// their scene depth is to its own, so a ridge in front of a cloud keeps a clean edge.
export function composeMaterial(uniforms: Record<string, IUniform>) {
  return new ShaderMaterial({
    uniforms,
    depthTest: false,
    depthWrite: false,
    vertexShader: fullscreenVertex,
    fragmentShader: `
      uniform sampler2D sceneColor;
      uniform sampler2D sceneDepth;
      uniform sampler2D remembered;
      uniform vec2 marchSize;
      varying vec2 vUv;

      void main() {
        float depth = texture(sceneDepth, vUv).r;
        vec2 at = vUv * marchSize - 0.5;
        vec2 corner = floor(at);
        vec2 part = at - corner;
        vec4 cloud = vec4(0.0);
        float total = 0.0;
        for (int i = 0; i < 4; i++) {
          vec2 offset = vec2(float(i & 1), float(i >> 1));
          vec2 uv = (corner + offset + 0.5) / marchSize;
          vec2 share = mix(1.0 - part, part, offset);
          float weight = share.x * share.y / (0.0005 + abs(texture(sceneDepth, uv).r - depth));
          cloud += texture(remembered, uv) * weight;
          total += weight;
        }
        cloud /= max(total, 1e-6);
        gl_FragColor = vec4(texture(sceneColor, vUv).rgb * cloud.a + cloud.rgb, 1.0);
      }
    `,
  });
}
