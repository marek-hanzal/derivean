import type { DepthTexture, Matrix4, UniformArrayNode, UniformNode, Vector4 } from "three/webgpu";

// What a surface reads to know whether the sun reaches it, per cascade and all in render space.
export interface ShadowNodes {
  // Depth seen from the sun: the nearest face that looks away from it.
  maps: DepthTexture[];
  // Render space to the map's clip space; the bias is already in it.
  matrices: UniformArrayNode<"mat4">;
  // The square of ground the cascade's own levels fill, as min x, min z, max x, max z; empty while the cascade
  // holds nothing.
  squares: UniformArrayNode<"vec4">;
  // xyz the unit vector towards the sun the map was drawn for, w the depth a metre towards it adds.
  rays: UniformArrayNode<"vec4">;
  // For the pass that draws a map: the finest level drawn into it.
  finest: UniformNode<"float", number>;
}

export interface ShadowUniforms {
  matrices: Matrix4[];
  squares: Vector4[];
  rays: Vector4[];
}
