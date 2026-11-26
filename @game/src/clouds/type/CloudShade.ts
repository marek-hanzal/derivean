import type { Texture, Vector4 } from "three";

// What a lit surface needs to stand in a cloud's shadow.
export interface CloudShade {
  // Share of direct sunlight that reaches each place of a level plane around the eye.
  cloudShadowMap: { value: Texture };
  // xy: render-space centre of the map, z: its side in metres, w: render-space height of its plane. A side of
  // zero means there is no map and everything is lit.
  cloudShadowPlace: { value: Vector4 };
}
