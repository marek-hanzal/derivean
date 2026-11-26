import { BackSide, Mesh, ShaderMaterial, SphereGeometry } from "three";
import { skyRadiance } from "../constant/skyRadiance";
import type { SkyLight } from "../type/SkyLight";

// The clear sky and the sun's disc. The dome ignores depth and draws first, so its radius is unrelated to the
// view distance; keep it centred on the camera. Clouds are composed over it afterwards.
export function dome(light: SkyLight) {
  const mesh = new Mesh(new SphereGeometry(100, 32, 16), new ShaderMaterial({
    side: BackSide,
    depthWrite: false,
    depthTest: false,
    uniforms: { ...light },
    vertexShader: `
      varying vec3 ray;
      void main() {
        ray = position;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: `
      uniform vec3 sunLight;
      varying vec3 ray;
      ${skyRadiance}
      void main() {
        vec3 direction = normalize(ray);
        float disc = smoothstep(0.99990, 0.99996, dot(direction, skySun)) * step(0.0, direction.y + 0.02);
        gl_FragColor = vec4(skyRadiance(direction) + sunLight * disc * 14.0, 1.0);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }
    `,
  }));
  mesh.renderOrder = -1;
  mesh.frustumCulled = false;
  return mesh;
}
