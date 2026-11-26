import type { DataArrayTexture, DataTexture, Data3DTexture, Vector2, Vector3, Vector4 } from "three";

// What the density function reads. Positions are render space; the offsets carry where the render origin lies in
// each world-anchored field, so nothing here grows with distance from the world origin.
export interface CloudUniforms {
  cloudBands: { value: DataArrayTexture | null };
  cloudShapes: { value: Data3DTexture };
  cloudDetails: { value: Data3DTexture };
  cloudLevels: { value: DataTexture };
  // xy: place of the render origin on the weather grid in planet repeats, z: 1 / planet span, w: cells a side.
  cloudGrid: { value: Vector4 };
  // World height of render-space y = 0.
  cloudDatum: { value: number };
  // Per band: lowest base and highest top near the eye, and z above zero when the band holds cloud there.
  cloudRange: { value: Vector4[] };
  // Place of the render origin in the broad shapes, in repeats.
  cloudSteer: { value: Vector2 };
  // Per band, in repeats of its small shapes with the wind's drift taken off: xy plain, zw streaked.
  cloudDrift: { value: Vector4[] };
  // Unit direction of each band's wind.
  cloudWind: { value: Vector2[] };
  // Render-space eye.
  cloudEye: { value: Vector3 };
}
