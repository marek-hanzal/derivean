import { GroundRepeat } from "./grain";

// An octave of noise in cells of `cell` metres on the world's axes, and one on the turned axes. The turn carries
// a step of GroundRepeat along x or z onto three and four fifths of it, so a turned octave repeats after a fifth
// of GroundRepeat; every cell here divides that.
const straight = (cell: number) =>
  `groundNoise(at / ${cell.toFixed(1)}, vec3(${(GroundRepeat / cell).toFixed(1)}))`;
const turned = (cell: number, offset: number) => {
  const along = (GroundRepeat / 5 / cell).toFixed(1);
  return `groundNoise(turned / ${cell.toFixed(1)} + ${offset.toFixed(1)}, vec3(${along}, ${(GroundRepeat / cell).toFixed(1)}, ${along}))`;
};

// What makes a stretch of one material look like ground instead of paint, shared by voxels and far terrain so the
// two agree wherever they meet. None of it is simulated: the material and its colour come from the world, and this
// only varies that colour quietly in world space (stains over hundreds of metres, mottling over tens, grain over
// metres) and gives the far terrain's light the same unevenness as a relief too small for its height field.
// Nothing here colours ground by its slope, and height enters only as the thin beds drawn within one rock.
//
// The finer patterns are laid on axes turned off the voxel grid's by a 3-4-5 triangle, which keeps the grain
// from lining up with cube edges and patch borders and still lets the pattern survive the render origin's steps.
export const ground = `
  uniform float plain;
  uniform vec3 groundAnchor;
  const vec3 plainTone = vec3(0.6);

  float groundHash(vec3 cell) {
    cell = fract(cell * 0.1031);
    cell += dot(cell, cell.yzx + 33.33);
    return fract((cell.x + cell.y) * cell.z);
  }

  // Value noise in cells of one unit that repeats after \`repeat\` cells along each axis.
  float groundNoise(vec3 at, vec3 repeat) {
    vec3 cell = floor(at);
    vec3 part = fract(at);
    part = part * part * (3.0 - 2.0 * part);
    vec2 unit = vec2(0.0, 1.0);
    float low = mix(
      mix(groundHash(mod(cell, repeat)), groundHash(mod(cell + unit.yxx, repeat)), part.x),
      mix(groundHash(mod(cell + unit.xyx, repeat)), groundHash(mod(cell + unit.yyx, repeat)), part.x), part.y);
    float high = mix(
      mix(groundHash(mod(cell + unit.xxy, repeat)), groundHash(mod(cell + unit.yxy, repeat)), part.x),
      mix(groundHash(mod(cell + unit.xyy, repeat)), groundHash(mod(cell + unit.yyy, repeat)), part.x), part.y);
    return mix(low, high, part.z);
  }

  // The colour of a material at a render-space point. \`loose\` runs from solid rock at 0 to loose grains at 1;
  // \`depth\` is the view depth, past which variation finer than a pixel is left out. \`rise\` comes back as the
  // same unevenness in metres of height, for groundBump.
  vec3 groundColor(vec3 base, vec3 p, float loose, float depth, out float rise) {
    vec3 at = p + groundAnchor;
    vec3 turned = vec3((3.0 * at.x + 4.0 * at.z) / 5.0, at.y, (3.0 * at.z - 4.0 * at.x) / 5.0);
    float stain = ${straight(256)} - 0.5;
    float mottle = (${turned(25.6, 3.7)} - 0.5) * (1.0 - smoothstep(5000.0, 16000.0, depth));
    float patchy = (${turned(6.4, 9.1)} - 0.5) * (1.0 - smoothstep(1200.0, 4000.0, depth));
    float fine = (${turned(1.6, 5.3)} - 0.5) * (1.0 - smoothstep(120.0, 400.0, depth));
    // Rock keeps thin beds within one material: noise drawn out sixteen metres along the ground and half a metre
    // up. Loose ground has none.
    float bedded = (groundNoise(vec3(at.x / 16.0, at.y * 2.0, at.z / 16.0), vec3(${(GroundRepeat / 16).toFixed(1)}, ${(GroundRepeat * 2).toFixed(1)}, ${(GroundRepeat / 16).toFixed(1)})) - 0.5)
      * (1.0 - smoothstep(30.0, 250.0, depth));
    float tone = 1.0 + stain * 0.3 + mottle * mix(0.26, 0.34, loose) + patchy * 0.3 + fine * 0.26 + bedded * mix(0.3, 0.0, loose);
    rise = (mottle * 3.0 + patchy * 1.0 + fine * 0.3) * mix(1.0, 0.5, loose);
    // Stains run warm where they darken and pale where they bleach, as weathered ground does.
    vec3 hue = mix(vec3(1.05, 0.99, 0.92), vec3(0.97, 1.0, 1.04), clamp(0.5 + stain * 1.6 + mottle * 0.8, 0.0, 1.0));
    return base * tone * hue;
  }

  // A view-space normal tilted by a relief of \`rise\` metres over the surface at view-space \`at\`, from how
  // the rise changes from pixel to pixel.
  vec3 groundBump(vec3 normal, vec3 at, float rise) {
    vec3 alongX = dFdx(at);
    vec3 alongY = dFdy(at);
    vec3 acrossX = cross(alongY, normal);
    vec3 acrossY = cross(normal, alongX);
    float area = dot(alongX, acrossX);
    vec3 slope = sign(area) * (dFdx(rise) * acrossX + dFdy(rise) * acrossY);
    return normalize(abs(area) * normal - slope);
  }
`;
