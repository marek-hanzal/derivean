import type { Matrix4, Texture } from "three";

// What a surface needs to stand in the shadow a mountain casts.
export interface TerrainShade {
  // Depth of the far terrain seen from the sun.
  terrainShadowMap: { value: Texture };
  // Render space to that view: xy across the map and z its depth, all 0…1.
  terrainShadowMatrix: { value: Matrix4 };
  // 1 while the map holds shadows to draw, 0 when the sun is down, under plain shading, or before its first draw.
  terrainShadowOn: { value: number };
}
