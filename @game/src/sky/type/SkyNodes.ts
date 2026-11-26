import type { Color, UniformNode, Vector2, Vector3, Vector4 } from "three/webgpu";

// The light of the hour and the air, as the uniforms every node material shares. Colours are linear radiance.
export interface SkyNodes {
  // The clear sky (SkyLight): unit vector towards the sun, zenith over horizon, the low sun's warm light, and
  // the sun's own colour for its disc.
  skySun: UniformNode<"vec3", Vector3>;
  skyZenith: UniformNode<"color", Color>;
  skyHorizon: UniformNode<"color", Color>;
  skyGlow: UniformNode<"color", Color>;
  sunDisc: UniformNode<"color", Color>;
  // What lights a surface: direct light and where it comes from, and the light of the sky above and of the
  // ground below, between which a surface takes its share by how far it faces up.
  sunDirection: UniformNode<"vec3", Vector3>;
  sunLight: UniformNode<"color", Color>;
  skyLight: UniformNode<"color", Color>;
  groundLight: UniformNode<"color", Color>;
  // The air (Air): render-space height of the sea, share of the air's thickness that is drawn, metres between
  // which the last of the view is given to the sky, and the cloud over the eye.
  airSea: UniformNode<"float", number>;
  airShare: UniformNode<"float", number>;
  airEnd: UniformNode<"vec2", Vector2>;
  airVeil: UniformNode<"vec4", Vector4>;
  // 1 for plain relief shading.
  plain: UniformNode<"float", number>;
}
