// Side in metres and resolution of the map of cloud shadows around the eye, and how often it is redrawn.
export const ShadowSpan = 96000;
export const ShadowSize = 1024;
export const ShadowMs = 200;

// Share of direct sunlight a surface point receives through the clouds, from the CloudShade and SkyLight
// uniforms. The point is carried back along the sun's rays to the map's plane, so a shadow lies where the cloud
// that casts it stands in the way, at any height of ground.
export const cloudShadow = `
  uniform sampler2D cloudShadowMap;
  uniform vec4 cloudShadowPlace;
  float cloudShadow(vec3 p, vec3 towardsSun) {
    if (cloudShadowPlace.z <= 0.0 || towardsSun.y <= 0.0) return 1.0;
    vec2 foot = p.xz - towardsSun.xz * ((p.y - cloudShadowPlace.w) / max(towardsSun.y, 0.12));
    vec2 uv = (foot - cloudShadowPlace.xy) / cloudShadowPlace.z + 0.5;
    vec2 edge = min(uv, 1.0 - uv);
    // Four taps round the point: a shadow's edge is softer than a texel of the map.
    const float tap = ${(0.75 / ShadowSize).toFixed(6)};
    float lit = texture2D(cloudShadowMap, uv + vec2(tap, tap)).r + texture2D(cloudShadowMap, uv + vec2(-tap, tap)).r
      + texture2D(cloudShadowMap, uv + vec2(tap, -tap)).r + texture2D(cloudShadowMap, uv - vec2(tap, tap)).r;
    return mix(1.0, lit * 0.25, smoothstep(0.0, 0.06, min(edge.x, edge.y)));
  }
`;
