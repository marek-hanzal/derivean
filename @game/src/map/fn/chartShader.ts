import { ChartWater } from "../constant/chart";
import { BareCover, SnowWarmth } from "../constant/map";

const float = (value: number) => value.toFixed(3);

// Bytes of the chart's uniforms (ChartUniforms below), as ChartSurface writes them.
export const ChartUniformBytes = 48;

// Colours a chart: hypsometric tints under oblique light from the north-west, snow, ice and bare rock, contour
// lines, streams and standing water. Elevation and flow are read through a cubic B-spline of the 4 × 4 texels
// around a point, so relief, contours and streams stay smooth however far the raster is magnified, and every
// line is antialiased by the screen-space rate of the field it is drawn from.
//
// The rasters come in two sheets (ChartSurface): `relief` holds ground, discharge, level and waters, the texels
// the spline reads; `climate` holds the water's surface, ice, warmth and cover, read between the four texels
// around the point. Level and waters are their codes as floats.
//
// Pixels further than two pixels beyond the chart return paper at once. Every pixel nearer runs the same code, so
// the screen-space rates of a pixel on the chart are those of neighbours that ran it too: derivatives are taken
// past a return that depends on the pixel, which WGSL's uniformity analysis would refuse, hence the diagnostic on
// `paint`. The rates are taken over 2 × 2 blocks of pixels counted from the top-left, as WebGPU counts them.
export const ChartWgsl = `

struct ChartUniforms {
  texelOrigin: vec2f,       // chart texel under the top-left corner of the viewport
  texelPerPixel: f32,
  size: i32,                // texels per side; 0 while there is no chart
  pitch: f32,               // metres per texel
  seaLevel: f32,
  interval: f32,            // metres between contour lines
  relief: f32,              // vertical exaggeration of the shading
  streamFloor: f32,         // m³/yr below which a stream is not drawn
  streamFloorLog: f32,      // its log2
  markPlanet: f32,          // 1 while the chart could show finer data than the planet's
  pixelRatio: f32,
}

@group(0) @binding(0) var<uniform> chart: ChartUniforms;
@group(0) @binding(1) var relief: texture_2d<f32>;
@group(0) @binding(2) var climate: texture_2d<f32>;

@vertex fn corner(@builtin(vertex_index) index: u32) -> @builtin(position) vec4f {
  return vec4f(select(-1.0, 3.0, index == 1u), select(-1.0, 3.0, index == 2u), 0.0, 1.0);
}

const Paper = vec3f(0.075, 0.110, 0.098);
const Lake = vec3f(0.620, 0.800, 0.900);
const Stream = vec3f(0.240, 0.470, 0.720);
const Shore = vec3f(0.300, 0.470, 0.640);
const Snow = vec3f(0.955, 0.965, 0.975);
const Ice = vec3f(0.860, 0.930, 0.970);
const Rock = vec3f(0.700, 0.680, 0.650);
const Contour = vec3f(0.420, 0.300, 0.200);
const Light = vec3f(-0.530, 0.660, -0.530);   // towards the sun: north-west, 41° up

const LandAt = array<f32, 9>(0.0, 100.0, 300.0, 700.0, 1200.0, 2000.0, 3000.0, 4200.0, 5500.0);
const LandTint = array<vec3f, 9>(
  vec3f(0.675, 0.784, 0.627), vec3f(0.737, 0.816, 0.643), vec3f(0.839, 0.863, 0.675), vec3f(0.910, 0.871, 0.698), vec3f(0.886, 0.800, 0.627),
  vec3f(0.808, 0.698, 0.557), vec3f(0.737, 0.659, 0.588), vec3f(0.839, 0.816, 0.800), vec3f(0.961, 0.961, 0.961));
const SeaAt = array<f32, 6>(0.0, 50.0, 200.0, 1000.0, 3000.0, 6000.0);
const SeaTint = array<vec3f, 6>(
  vec3f(0.765, 0.875, 0.922), vec3f(0.690, 0.835, 0.906), vec3f(0.596, 0.776, 0.878), vec3f(0.455, 0.659, 0.812), vec3f(0.337, 0.541, 0.737),
  vec3f(0.255, 0.427, 0.647));

fn landTint(above: f32) -> vec3f {
  var color = LandTint[0];
  for (var i = 1; i < 9; i++) { color = mix(color, LandTint[i], clamp((above - LandAt[i - 1]) / (LandAt[i] - LandAt[i - 1]), 0.0, 1.0)); }
  return color;
}

fn seaTint(depth: f32) -> vec3f {
  var color = SeaTint[0];
  for (var i = 1; i < 6; i++) { color = mix(color, SeaTint[i], clamp((depth - SeaAt[i - 1]) / (SeaAt[i] - SeaAt[i - 1]), 0.0, 1.0)); }
  return color;
}

fn within(texel: vec2i) -> vec2i { return clamp(texel, vec2i(0), vec2i(chart.size - 1)); }
fn code(value: f32) -> i32 { return i32(value + 0.5); }

fn spline(t: f32) -> vec4f {
  let s = 1.0 - t;
  return vec4f(s * s * s, 3.0 * t * t * t - 6.0 * t * t + 4.0, -3.0 * t * t * t + 3.0 * t * t + 3.0 * t + 1.0, t * t * t) / 6.0;
}
fn splineSlope(t: f32) -> vec4f {
  let s = 1.0 - t;
  return vec4f(-0.5 * s * s, 1.5 * t * t - 2.0 * t, -1.5 * t * t + t + 0.5, 0.5 * t * t);
}

// 1 on a line of half-width \`reach\` pixels around zero of a field that changes by \`rate\` per pixel.
fn stroke(away: f32, rate: f32, reach: f32) -> f32 {
  return 1.0 - smoothstep(reach - 0.5, reach + 0.5, abs(away) / max(rate, 1e-6));
}

@diagnostic(off, derivative_uniformity) @fragment fn paint(@builtin(position) position: vec4f) -> @location(0) vec4f {
  let pixel = position.xy;
  let at = chart.texelOrigin + pixel * chart.texelPerPixel;
  let reach = 2.0 * chart.texelPerPixel;
  if (chart.size == 0 || any(at < vec2f(-reach)) || any(at > vec2f(f32(chart.size) + reach))) { return vec4f(Paper, 1.0); }
  // Pixels just beyond the chart are coloured like the rest and painted over at the end, for the rates of those
  // on it.
  let charted = all(at >= vec2f(0.0)) && all(at <= vec2f(f32(chart.size)));
  let centred = at - 0.5;
  let corner = vec2i(floor(centred));
  let t = centred - floor(centred);

  let wx = spline(t.x);
  let wz = spline(t.y);
  let sx = splineSlope(t.x);
  let sz = splineSlope(t.y);
  let here = code(textureLoad(relief, within(vec2i(floor(at))), 0).z);
  var height = 0.0;
  var slopeX = 0.0;
  var slopeZ = 0.0;
  var flow = 0.0;
  var coarse = 0.0;
  var foreign = 0.0;
  // The waters of the 4 × 4 texels, for the four around the point.
  var kinds: array<i32, 16>;
  for (var j = 0; j < 4; j++) {
    for (var i = 0; i < 4; i++) {
      let texel = textureLoad(relief, within(corner + vec2i(i - 1, j - 1)), 0);
      let weight = wx[i] * wz[j];
      let detail = code(texel.z);
      kinds[j * 4 + i] = code(texel.w);
      coarse += select(0.0, weight, detail == 0);
      foreign += select(0.0, weight, detail != here);
      let ground = texel.x;
      height += weight * ground;
      slopeX += sx[i] * wz[j] * ground;
      slopeZ += wx[i] * sz[j] * ground;
      // A stream is a chain of single texels: full strength over three decades of flow, so greater rivers draw
      // wider.
      let discharge = texel.y;
      flow += weight * select(0.0, 0.6 + 0.4 * clamp((log2(discharge) - chart.streamFloorLog) / 10.0, 0.0, 1.0), discharge >= chart.streamFloor);
    }
  }

  // The four texels around the point, blended linearly: standing water, climate and cover.
  var wet = 0.0;
  var lake = 0.0;
  var level = 0.0;
  var ice = 0.0;
  var warmth = 0.0;
  var cover = 0.0;
  for (var j = 0; j < 2; j++) {
    for (var i = 0; i < 2; i++) {
      let texel = textureLoad(climate, within(corner + vec2i(i, j)), 0);
      let weight = select(t.x, 1.0 - t.x, i == 0) * select(t.y, 1.0 - t.y, j == 0);
      let kind = kinds[(j + 1) * 4 + i + 1];
      if (kind == ${ChartWater.Sea} || kind == ${ChartWater.Lake}) {
        wet += weight;
        level += weight * texel.x;
        lake += select(0.0, weight, kind == ${ChartWater.Lake});
      }
      ice += weight * texel.y;
      warmth += weight * texel.z;
      cover += weight * texel.w;
    }
  }

  let slope = vec2f(slopeX, slopeZ) / chart.pitch * chart.relief;
  let lit = dot(normalize(vec3f(-slope.x, 1.0, -slope.y)), Light);
  // Two levels of detail seldom agree on the height of their shared edge. The step between them is no cliff:
  // across it the relief is drawn flat and without contours.
  let seam = smoothstep(0.02, 0.3, foreign);
  let shade = mix(clamp(1.0 + 1.15 * (lit - Light.y), 0.5, 1.25), 1.0, seam);

  let above = height - chart.seaLevel;
  var color = landTint(above);
  let bare = (1.0 - smoothstep(${float(BareCover[0])}, ${float(BareCover[1])}, cover)) * (1.0 - coarse);
  color = mix(color, Rock, 0.3 * bare);
  let frozen = max(1.0 - smoothstep(${float(SnowWarmth[0])}, ${float(SnowWarmth[1])}, warmth), 0.0);
  color = mix(color, Snow, 0.9 * frozen);
  let glacier = smoothstep(0.5, 3.0, ice);
  color = mix(color, Ice, glacier);
  if (shade < 1.0) { color = mix(color, color * vec3f(0.42, 0.40, 0.50), (1.0 - shade) / 0.5 * 0.7); }
  else { color = mix(color, vec3f(1.0, 0.99, 0.94), (shade - 1.0) * 1.6); }

  // Contour lines every interval metres above the sea, every fifth one heavier. Lines closer than a few pixels
  // would merge into a tone, so they fade out before that.
  let turns = above / chart.interval;
  let rate = fwidth(turns);
  let fine = stroke(fract(turns + 0.5) - 0.5, rate, 0.5 * chart.pixelRatio) * (1.0 - smoothstep(0.16, 0.36, rate * chart.pixelRatio));
  let index = stroke(fract(turns / 5.0 + 0.5) - 0.5, rate / 5.0, 0.85 * chart.pixelRatio) * (1.0 - smoothstep(0.16, 0.36, rate / 5.0 * chart.pixelRatio));
  let ink = mix(Contour, vec3f(0.36, 0.46, 0.60), max(frozen, glacier));
  color = mix(color, ink, (1.0 - seam) * step(1e-5, rate) * max(0.30 * fine, 0.52 * index));

  let bank = max(fwidth(flow), 1e-4);
  color = mix(color, Stream, 0.92 * smoothstep(0.25 - bank, 0.25 + bank, flow));

  // Standing water ends where the smoothed ground rises through its level, not at the edge of a texel: the
  // texels only say which water is near and how high it stands.
  let depth = level / max(wet, 1e-3) - height;
  let coast = max(fwidth(depth), 1e-4);
  let near = smoothstep(0.0, 0.25, wet);
  if (wet > 0.0) {
    let fill = mix(seaTint(max(0.0, depth)), Lake, lake / wet) * mix(1.0, shade, 0.22);
    color = mix(color, fill, near * smoothstep(-coast, coast, depth));
    color = mix(color, Shore, 0.55 * near * (1.0 - seam) * stroke(depth, coast, 0.6 * chart.pixelRatio));
  }

  // Ground the simulation only knows at planet scale is greyed and hatched while finer data could show.
  let planet = chart.markPlanet * smoothstep(0.4, 0.6, coarse);
  if (planet > 0.0) {
    let period = 9.0 * chart.pixelRatio;
    let hatch = stroke((fract((pixel.x + pixel.y) / period) - 0.5) * period, 1.0, 0.45 * chart.pixelRatio);
    let muted = mix(color, vec3f(dot(color, vec3f(0.30, 0.59, 0.11))), 0.4);
    color = mix(color, mix(muted, muted * 0.80, 0.55 * hatch), planet);
  }
  return vec4f(select(Paper, color, charted), 1.0);
}
`;

// The chart drawn at one pixel per CSS pixel stretched onto the canvas, \`canvas\` pixels, filtered linearly: a
// canvas pixel's centre maps onto the same point of the reduced chart.
export const StretchWgsl = `
@group(0) @binding(0) var<uniform> canvas: vec2f;
@group(0) @binding(1) var reduced: texture_2d<f32>;
@group(0) @binding(2) var stretched: sampler;

@vertex fn corner(@builtin(vertex_index) index: u32) -> @builtin(position) vec4f {
  return vec4f(select(-1.0, 3.0, index == 1u), select(-1.0, 3.0, index == 2u), 0.0, 1.0);
}

@fragment fn stretch(@builtin(position) position: vec4f) -> @location(0) vec4f {
  return textureSampleLevel(reduced, stretched, position.xy / canvas, 0.0);
}
`;
