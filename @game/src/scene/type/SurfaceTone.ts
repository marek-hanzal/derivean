import type { Vector3 } from "three";

// Uniforms shared by every ground surface.
export interface SurfaceTone {
  // 1 for plain relief shading.
  plain: { value: number };
  grainCells: { value: number };
  voxelSize: { value: number };
  // The render origin within one repeat of the ground's variation: a render-space position plus this is the same
  // for a place in the world wherever the origin lies.
  groundAnchor: { value: Vector3 };
}
