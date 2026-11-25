import { BackSide, Color, Mesh, ShaderMaterial, SphereGeometry, Vector3 } from "three";

export const sunlight = new Vector3(-13, 12, 7);

export function sky(horizon: Color) {
  return new Mesh(new SphereGeometry(100, 32, 16), new ShaderMaterial({
    side: BackSide,
    depthWrite: false,
    uniforms: {
      horizon: { value: horizon },
      upper: { value: new Color("#59777d") },
      sun: { value: sunlight.clone().normalize() },
    },
    vertexShader: `
      varying vec3 ray;
      void main() {
        ray = position;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }
    `,
    fragmentShader: `
      uniform vec3 horizon;
      uniform vec3 upper;
      uniform vec3 sun;
      varying vec3 ray;
      void main() {
        vec3 direction = normalize(ray);
        float height = clamp(direction.y, 0.0, 1.0);
        float scatter = max(dot(direction, sun), 0.0);
        vec3 color = mix(horizon, upper, smoothstep(0.0, 0.69, height));
        color += vec3(0.19, 0.11, 0.04) * pow(scatter, 13.0) * smoothstep(0.0, 0.28, height);
        color += vec3(2.0, 1.55, 0.9) * smoothstep(0.99922, 0.99955, scatter);
        gl_FragColor = vec4(color, 1.0);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }
    `,
  }));
}
