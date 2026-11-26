import type { Vector3 } from "three";

// The render-space box inside which voxel chunks own the terrain and far patches stand back. Outside its
// footprint the roles swap. An empty box (min above max) gives all of the terrain to the patches.
export interface HandOff {
  boxMin: { value: Vector3 };
  boxMax: { value: Vector3 };
}
