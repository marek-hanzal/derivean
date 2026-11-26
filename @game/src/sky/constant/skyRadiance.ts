// Radiance of the clear sky in a direction, from the SkyLight uniforms: zenith over horizon, the warm band on the
// low sun's side, and the aureole around it.
export const skyRadiance = `
  uniform vec3 skySun;
  uniform vec3 skyZenith;
  uniform vec3 skyHorizon;
  uniform vec3 skyGlow;
  vec3 skyRadiance(vec3 direction) {
    float up = max(direction.y, 0.0);
    float toSun = dot(direction, skySun);
    vec3 color = mix(skyHorizon, skyZenith, 1.0 - exp(-3.4 * up));
    color += skyGlow * exp(-6.0 * up) * pow(max(toSun * 0.5 + 0.5, 0.0), 4.0) * 0.8;
    color += skyGlow * pow(max(toSun, 0.0), 32.0) * 0.5;
    return color;
  }
`;
