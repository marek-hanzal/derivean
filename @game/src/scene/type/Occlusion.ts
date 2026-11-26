import type { Data3DTexture, Vector2, Vector4 } from "three";

// What the voxel surface needs to darken its creases.
export interface Occlusion {
  // Which voxels near the player are solid surface, one texel a voxel, wrapping.
  voxelOcclusion: { value: Data3DTexture };
  // xyz: where the render origin lies in the volume, in volumes; w: volumes per metre.
  occlusionPlace: { value: Vector4 };
  // x: metres from the eye within which the volume is filled; y: how much of the darkening is on, 0…1.
  occlusionReach: { value: Vector2 };
}
