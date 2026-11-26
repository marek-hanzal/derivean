import type { Color, Vector3 } from "three/webgpu";

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
  // Light from the clear sky onto level ground, from the air's own scattering.
  skyAmbient: { value: Color };
  // Light from the dome as it is drawn (its zenith and horizon colours), which the clouds seen against it take.
  domeLight: { value: Color };
}
