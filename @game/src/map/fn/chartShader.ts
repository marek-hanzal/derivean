import { ChartWater, Rasters } from "../constant/chart";
import { BareCover, SnowWarmth } from "../constant/map";

const float = (value: number) => value.toFixed(3);

export const VertexShader = `#version 300 es
const vec2 Corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
void main() { gl_Position = vec4(Corners[gl_VertexID], 0.0, 1.0); }
`;

// Colours a chart: hypsometric tints under oblique light from the north-west, snow, ice and bare rock, contour
// lines, streams and standing water. Elevation and flow are read through a cubic B-spline of the 4 × 4 texels
// around a point, so relief, contours and streams stay smooth however far the raster is magnified, and every
// line is antialiased by the screen-space rate of the field it is drawn from.
export const FragmentShader = `#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

uniform sampler2D ${Rasters.map((name) => `u_${name}`).join(", ")};
uniform vec2 uResolution;      // device pixels
uniform vec2 uTexelOrigin;     // chart texel under the top-left corner of the viewport
uniform float uTexelPerPixel;
uniform int uSize;             // texels per side; 0 while there is no chart
uniform float uPitch;          // metres per texel
uniform float uSeaLevel;
uniform float uInterval;       // metres between contour lines
uniform float uRelief;         // vertical exaggeration of the shading
uniform float uStreamFloor;    // m³/yr below which a stream is not drawn
uniform float uMarkPlanet;     // 1 while the chart could show finer data than the planet's
uniform float uPixelRatio;

out vec4 outColor;

const vec3 Paper = vec3(0.075, 0.110, 0.098);
const vec3 Lake = vec3(0.620, 0.800, 0.900);
const vec3 Stream = vec3(0.240, 0.470, 0.720);
const vec3 Shore = vec3(0.300, 0.470, 0.640);
const vec3 Snow = vec3(0.955, 0.965, 0.975);
const vec3 Ice = vec3(0.860, 0.930, 0.970);
const vec3 Rock = vec3(0.700, 0.680, 0.650);
const vec3 Contour = vec3(0.420, 0.300, 0.200);
const vec3 Light = vec3(-0.530, 0.660, -0.530);   // towards the sun: north-west, 41° up

const float LandAt[9] = float[9](0.0, 100.0, 300.0, 700.0, 1200.0, 2000.0, 3000.0, 4200.0, 5500.0);
const vec3 LandTint[9] = vec3[9](
  vec3(0.675, 0.784, 0.627), vec3(0.737, 0.816, 0.643), vec3(0.839, 0.863, 0.675), vec3(0.910, 0.871, 0.698), vec3(0.886, 0.800, 0.627),
  vec3(0.808, 0.698, 0.557), vec3(0.737, 0.659, 0.588), vec3(0.839, 0.816, 0.800), vec3(0.961, 0.961, 0.961));
const float SeaAt[6] = float[6](0.0, 50.0, 200.0, 1000.0, 3000.0, 6000.0);
const vec3 SeaTint[6] = vec3[6](
  vec3(0.765, 0.875, 0.922), vec3(0.690, 0.835, 0.906), vec3(0.596, 0.776, 0.878), vec3(0.455, 0.659, 0.812), vec3(0.337, 0.541, 0.737),
  vec3(0.255, 0.427, 0.647));

vec3 landTint(float above) {
  vec3 color = LandTint[0];
  for (int i = 1; i < 9; i++) color = mix(color, LandTint[i], clamp((above - LandAt[i - 1]) / (LandAt[i] - LandAt[i - 1]), 0.0, 1.0));
  return color;
}

vec3 seaTint(float depth) {
  vec3 color = SeaTint[0];
  for (int i = 1; i < 6; i++) color = mix(color, SeaTint[i], clamp((depth - SeaAt[i - 1]) / (SeaAt[i] - SeaAt[i - 1]), 0.0, 1.0));
  return color;
}

float read(sampler2D raster, ivec2 texel) { return texelFetch(raster, clamp(texel, ivec2(0), ivec2(uSize - 1)), 0).r; }
int code(sampler2D raster, ivec2 texel) { return int(read(raster, texel) * 255.0 + 0.5); }

vec4 spline(float t) {
  float s = 1.0 - t;
  return vec4(s * s * s, 3.0 * t * t * t - 6.0 * t * t + 4.0, -3.0 * t * t * t + 3.0 * t * t + 3.0 * t + 1.0, t * t * t) / 6.0;
}
vec4 splineSlope(float t) {
  float s = 1.0 - t;
  return vec4(-0.5 * s * s, 1.5 * t * t - 2.0 * t, -1.5 * t * t + t + 0.5, 0.5 * t * t);
}

// 1 on a line of half-width \`reach\` device pixels around zero of a field that changes by \`rate\` per pixel.
float stroke(float away, float rate, float reach) {
  return 1.0 - smoothstep(reach - 0.5, reach + 0.5, abs(away) / max(rate, 1e-6));
}

void main() {
  vec2 pixel = vec2(gl_FragCoord.x, uResolution.y - gl_FragCoord.y);
  vec2 at = uTexelOrigin + pixel * uTexelPerPixel;
  if (uSize == 0) {
    outColor = vec4(Paper, 1.0);
    return;
  }
  // Pixels beyond the chart are coloured like the rest and painted over at the end: screen-space rates are only
  // defined where every pixel runs the same code.
  bool charted = all(greaterThanEqual(at, vec2(0.0))) && all(lessThanEqual(at, vec2(float(uSize))));
  vec2 centred = at - 0.5;
  ivec2 corner = ivec2(floor(centred));
  vec2 t = centred - floor(centred);

  vec4 wx = spline(t.x), wz = spline(t.y), sx = splineSlope(t.x), sz = splineSlope(t.y);
  int here = code(u_level, ivec2(floor(at)));
  float height = 0.0, slopeX = 0.0, slopeZ = 0.0, flow = 0.0, coarse = 0.0, foreign = 0.0;
  for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) {
    ivec2 texel = corner + ivec2(i - 1, j - 1);
    int detail = code(u_level, texel);
    coarse += detail == 0 ? wx[i] * wz[j] : 0.0;
    foreign += detail != here ? wx[i] * wz[j] : 0.0;
    float ground = read(u_ground, texel);
    height += wx[i] * wz[j] * ground;
    slopeX += sx[i] * wz[j] * ground;
    slopeZ += wx[i] * sz[j] * ground;
    // A stream is a chain of single texels: full strength over three decades of flow, so greater rivers draw wider.
    float discharge = read(u_discharge, texel);
    flow += wx[i] * wz[j] * (discharge >= uStreamFloor ? 0.6 + 0.4 * clamp(log2(discharge / uStreamFloor) / 10.0, 0.0, 1.0) : 0.0);
  }

  // The four texels around the point, blended linearly: standing water, climate and cover.
  float wet = 0.0, lake = 0.0, level = 0.0, ice = 0.0, warmth = 0.0, cover = 0.0;
  for (int j = 0; j < 2; j++) for (int i = 0; i < 2; i++) {
    ivec2 texel = corner + ivec2(i, j);
    float weight = (i == 0 ? 1.0 - t.x : t.x) * (j == 0 ? 1.0 - t.y : t.y);
    int kind = code(u_waters, texel);
    if (kind == ${ChartWater.Sea} || kind == ${ChartWater.Lake}) {
      wet += weight;
      level += weight * read(u_water, texel);
      lake += kind == ${ChartWater.Lake} ? weight : 0.0;
    }
    ice += weight * read(u_ice, texel);
    warmth += weight * read(u_warmth, texel);
    cover += weight * read(u_cover, texel);
  }

  vec2 slope = vec2(slopeX, slopeZ) / uPitch * uRelief;
  float lit = dot(normalize(vec3(-slope.x, 1.0, -slope.y)), Light);
  // Two levels of detail seldom agree on the height of their shared edge. The step between them is no cliff:
  // across it the relief is drawn flat and without contours.
  float seam = smoothstep(0.02, 0.3, foreign);
  float shade = mix(clamp(1.0 + 1.15 * (lit - Light.y), 0.5, 1.25), 1.0, seam);

  float above = height - uSeaLevel;
  vec3 color = landTint(above);
  float bare = (1.0 - smoothstep(${float(BareCover[0])}, ${float(BareCover[1])}, cover)) * (1.0 - coarse);
  color = mix(color, Rock, 0.3 * bare);
  float frozen = max(1.0 - smoothstep(${float(SnowWarmth[0])}, ${float(SnowWarmth[1])}, warmth), 0.0);
  color = mix(color, Snow, 0.9 * frozen);
  float glacier = smoothstep(0.5, 3.0, ice);
  color = mix(color, Ice, glacier);
  color = shade < 1.0 ? mix(color, color * vec3(0.42, 0.40, 0.50), (1.0 - shade) / 0.5 * 0.7) : mix(color, vec3(1.0, 0.99, 0.94), (shade - 1.0) * 1.6);

  // Contour lines every uInterval metres above the sea, every fifth one heavier. Lines closer than a few
  // pixels would merge into a tone, so they fade out before that.
  float turns = above / uInterval;
  float rate = fwidth(turns);
  float fine = stroke(fract(turns + 0.5) - 0.5, rate, 0.5 * uPixelRatio) * (1.0 - smoothstep(0.16, 0.36, rate * uPixelRatio));
  float index = stroke(fract(turns / 5.0 + 0.5) - 0.5, rate / 5.0, 0.85 * uPixelRatio) * (1.0 - smoothstep(0.16, 0.36, rate / 5.0 * uPixelRatio));
  vec3 ink = mix(Contour, vec3(0.36, 0.46, 0.60), max(frozen, glacier));
  color = mix(color, ink, (1.0 - seam) * step(1e-5, rate) * max(0.30 * fine, 0.52 * index));

  float bank = max(fwidth(flow), 1e-4);
  color = mix(color, Stream, 0.92 * smoothstep(0.25 - bank, 0.25 + bank, flow));

  // Standing water ends where the smoothed ground rises through its level, not at the edge of a texel: the
  // texels only say which water is near and how high it stands.
  float depth = level / max(wet, 1e-3) - height;
  float coast = max(fwidth(depth), 1e-4);
  float near = smoothstep(0.0, 0.25, wet);
  if (wet > 0.0) {
    vec3 fill = mix(seaTint(max(0.0, depth)), Lake, lake / wet) * mix(1.0, shade, 0.22);
    color = mix(color, fill, near * smoothstep(-coast, coast, depth));
    color = mix(color, Shore, 0.55 * near * (1.0 - seam) * stroke(depth, coast, 0.6 * uPixelRatio));
  }

  // Ground the simulation only knows at planet scale is greyed and hatched while finer data could show.
  float planet = uMarkPlanet * smoothstep(0.4, 0.6, coarse);
  if (planet > 0.0) {
    float period = 9.0 * uPixelRatio;
    float hatch = stroke((fract((pixel.x + pixel.y) / period) - 0.5) * period, 1.0, 0.45 * uPixelRatio);
    vec3 muted = mix(color, vec3(dot(color, vec3(0.30, 0.59, 0.11))), 0.4);
    color = mix(color, mix(muted, muted * 0.80, 0.55 * hatch), planet);
  }
  outColor = vec4(charted ? color : Paper, 1.0);
}
`;
