import {
  BufferGeometry, Float32BufferAttribute, Mesh, OrthographicCamera, type Material, type WebGLRenderer,
  type WebGLRenderTarget,
} from "three";

const geometry = new BufferGeometry();
geometry.setAttribute("position", new Float32BufferAttribute([-1, 3, 0, -1, -1, 0, 3, -1, 0], 3));
geometry.setAttribute("uv", new Float32BufferAttribute([0, 2, 0, 0, 2, 0], 2));
const mesh = new Mesh(geometry);
mesh.frustumCulled = false;
const camera = new OrthographicCamera(-1, 1, 1, -1, 0, 1);

export const fullscreenVertex = `
  varying vec2 vUv;
  void main() {
    vUv = uv;
    gl_Position = vec4(position.xy, 0.0, 1.0);
  }
`;

// Runs a fragment shader over a whole target, or over one layer of a volume.
export function fullscreen(renderer: WebGLRenderer, material: Material, target: WebGLRenderTarget | null, layer = 0) {
  mesh.material = material;
  renderer.setRenderTarget(target, layer);
  renderer.render(mesh, camera);
}
