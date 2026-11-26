// Side in metres and resolution of the map of shadows the far terrain casts on itself, and how seldom it is
// redrawn at most. It is what makes relief read under a low sun; nearer than a few of its texels the voxels' own
// shadow map takes over.
export const TerrainShadowSpan = 120000;
export const TerrainShadowSize = 4096;
export const TerrainShadowMs = 700;
// While far terrain keeps arriving its shadows wait this long, so a flight is not a redraw every moment.
export const TerrainShadowSettleMs = 4000;
// Metres the eye may move before the map is drawn again around it.
export const TerrainShadowDrift = 6000;
// Layer the far terrain is also on, so the sun's view can be drawn of it alone.
export const TerrainLayer = 1;

// Metres across one texel of the map.
export const TerrainShadowTexel = TerrainShadowSpan / TerrainShadowSize;

// Share of direct sunlight that reaches a render-space point past the terrain between it and the sun. Before the
// comparison the point is pushed off its own surface, the further the more the sun grazes it, which is what
// keeps slopes from shadowing themselves; relief smaller than a couple of texels casts nothing here. `fromEye`
// tells which way the surface faces.
export const terrainShadow = `
  uniform sampler2D terrainShadowMap;
  uniform mat4 terrainShadowMatrix;
  uniform float terrainShadowOn;
  float terrainShadow(vec3 p, vec3 towardsSun, vec3 fromEye) {
    if (terrainShadowOn < 0.5) return 1.0;
    vec3 facing = normalize(cross(dFdx(p), dFdy(p)));
    facing = dot(facing, fromEye) > 0.0 ? -facing : facing;
    float toSun = dot(facing, towardsSun);
    float grazing = sqrt(1.0 - min(toSun * toSun, 1.0));
    vec4 at = terrainShadowMatrix * vec4(p + facing * ${(TerrainShadowTexel * 2.0).toFixed(1)} * grazing + towardsSun * ${(TerrainShadowTexel * 0.6).toFixed(1)}, 1.0);
    vec2 edge = min(at.xy, 1.0 - at.xy);
    if (min(edge.x, edge.y) <= 0.0 || at.z >= 1.0) return 1.0;
    const float tap = ${(0.7 / TerrainShadowSize).toFixed(7)};
    float lit = step(at.z, texture2D(terrainShadowMap, at.xy + vec2(tap, tap)).r) + step(at.z, texture2D(terrainShadowMap, at.xy + vec2(-tap, tap)).r)
      + step(at.z, texture2D(terrainShadowMap, at.xy + vec2(tap, -tap)).r) + step(at.z, texture2D(terrainShadowMap, at.xy - vec2(tap, tap)).r);
    return mix(1.0, lit * 0.25, smoothstep(0.0, 0.05, min(edge.x, edge.y)));
  }
`;
