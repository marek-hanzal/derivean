import { CleanAir, CleanAirHeight, HazeAir, HazeAirHeight, VeilReach } from "./air";
import { skyRadiance } from "./skyRadiance";

const rgb = (values: readonly number[]) => `vec3(${values.map((value) => value.toExponential(3)).join(", ")})`;

// Aerial perspective: what distance does to a colour seen through the air. Light is lost along the way and the
// sky's own light scattered in, blue first, which is why far ranges turn into blue-grey silhouettes rank behind
// rank. Both the clean air and the haze in it thin with height, so a summit seen from a summit stays clearer than
// a plain seen across a plain. At the end of the view the ground has become the clear sky just above the horizon
// in its direction, which is what the dome draws there, so the terrain never ends in an edge. The cloud over the
// eye greys only the air near it.
export const airFade = `
  ${skyRadiance}
  uniform float airSea;
  uniform float airShare;
  uniform vec2 airEnd;
  uniform vec4 airVeil;

  // Mean density, against the sea's, along a straight path between two heights of air that thins by e every
  // \`scale\` metres.
  float airMean(float from, float to, float scale) {
    float low = exp(-max(from, -2000.0) / scale);
    float high = exp(-max(to, -2000.0) / scale);
    float climb = (max(to, -2000.0) - max(from, -2000.0)) / scale;
    return abs(climb) < 0.01 ? low * (1.0 - climb * 0.5) : (low - high) / climb;
  }

  // What the sky sends from a direction to something \`range\` metres off: the clear sky's radiance, or near
  // the eye under cloud that cloud's grey.
  vec3 airLight(vec3 direction, float range) {
    return mix(skyRadiance(direction), airVeil.xyz, airVeil.w * (1.0 - smoothstep(${VeilReach[0].toFixed(1)}, ${VeilReach[1].toFixed(1)}, range)));
  }

  vec3 airFade(vec3 color, vec3 fromEye) {
    float range = length(fromEye);
    float from = cameraPosition.y - airSea;
    float to = from + fromEye.y;
    vec3 depth = (${rgb(CleanAir)} * airMean(from, to, ${CleanAirHeight.toFixed(1)})
      + ${HazeAir.toExponential(3)} * airMean(from, to, ${HazeAirHeight.toFixed(1)})) * range * airShare;
    vec3 lost = max(1.0 - exp(-depth), smoothstep(airEnd.x, airEnd.y, range));
    // Just over the horizon the way the ground lies, also for ground straight below.
    vec3 low = normalize(vec3(fromEye.x + 1e-3, max(fromEye.y, 0.0) * 0.4 + length(fromEye.xz) * 0.03, fromEye.z));
    return mix(color, airLight(low, range), lost);
  }
`;
