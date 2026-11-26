import type { TextureNode, UniformNode, Vector2, Vector3, Vector4 } from "three/webgpu";

// What the density function reads (constant/density), as nodes. Positions are render space; the offsets carry
// where the render origin lies in each world-anchored field, so nothing here grows with distance from the world
// origin. The fields mean what the CloudSky struct says.
export interface CloudNodes {
  grid: UniformNode<"vec4", Vector4>;
  steer: UniformNode<"vec2", Vector2>;
  datum: UniformNode<"float", number>;
  ranges: UniformNode<"vec4", Vector4>[];
  drifts: UniformNode<"vec4", Vector4>[];
  wind01: UniformNode<"vec4", Vector4>;
  wind2: UniformNode<"vec2", Vector2>;
  // Render-space eye.
  eye: UniformNode<"vec3", Vector3>;
  bands: TextureNode;
  shapes: TextureNode;
  details: TextureNode;
  levels: TextureNode;
}
