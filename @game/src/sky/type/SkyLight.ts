import type { Color, Vector3 } from "three";

// The light of the hour, as uniforms every shader that needs the sky shares. Colours are linear radiance.
export interface SkyLight {
  // Unit vector towards the sun; below the horizon at night.
  skySun: { value: Vector3 };
  skyZenith: { value: Color };
  skyHorizon: { value: Color };
  // Warm light around the low sun.
  skyGlow: { value: Color };
  // Direct sunlight after its path through the air, times its strength; black at night.
  sunLight: { value: Color };
  // Light from the sky dome and from the ground it lights.
  skyAmbient: { value: Color };
  groundAmbient: { value: Color };
}
