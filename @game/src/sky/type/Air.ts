import type { Vector2, Vector4 } from "three";

// The air between the eye and what it sees, as uniforms every surface shader shares.
export interface Air {
  // Render-space height of the sea, where the air is thickest.
  airSea: { value: number };
  // Share of the air's real thickness that is drawn: 1, less under plain shading.
  airShare: { value: number };
  // Metres from the eye between which the last of the far terrain is given to the sky, whatever the air left of it.
  airEnd: { value: Vector2 };
  // Under the cloud over the eye: the radiance the sky sends down there (xyz) and the cloud's share of it (w).
  airVeil: { value: Vector4 };
}
