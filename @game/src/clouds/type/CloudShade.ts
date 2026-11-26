import type { Node, Texture, UniformNode, Vector4 } from "three/webgpu";

// What a lit surface needs to stand in a cloud's shadow.
export interface CloudShade {
  // Share of direct sunlight that reaches each place of a level plane around the eye.
  map: Texture;
  // xy: render-space centre of the map, z: its side in metres, w: render-space height of its plane. A side of zero
  // means there is no map and everything is lit.
  place: UniformNode<"vec4", Vector4>;
  // Unit vector towards the sun the map is drawn for.
  sun: Node<"vec3">;
  // Share of direct sunlight that gets through the cloud over the eye, taken for the ground beyond the map.
  beyond: UniformNode<"float", number>;
}
