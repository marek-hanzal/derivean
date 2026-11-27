import { BroadRepeat, DetailRepeat, SmallRepeats, StreakStretch } from "./repeats";

// Shares of the sky the cover levels are tabulated for (noiseVolumes).
export const CoverSteps = 32;

// What the density function reads besides its textures, as the fields of the WGSL struct CloudSky; every function
// that builds one takes them as parameters of the same names (fn/cloudArgs).
//   grid    xy: place of the render origin on the weather grid in planet repeats, z: 1 / planet span, w: cells a side
//   steer   place of the render origin in the broad shapes, in repeats
//   datum   world height of render-space y = 0
//   range#  per band: lowest base and highest top near the eye, and z above zero when the band holds cloud there
//   drift#  per band, in repeats of its small shapes with the wind's drift taken off: xy plain, zw streaked
//   wind01  unit direction of the wind of band 0 in xy and of band 1 in zw; wind2 that of band 2
export const CloudSkyFields = [
  ["grid", "vec4f"], ["steer", "vec2f"], ["datum", "f32"], ["range0", "vec4f"], ["range1", "vec4f"], ["range2", "vec4f"],
  ["drift0", "vec4f"], ["drift1", "vec4f"], ["drift2", "vec4f"], ["wind01", "vec4f"], ["wind2", "vec2f"],
] as const;

const CloudSkyStruct = `struct CloudSky { ${CloudSkyFields.map(([name, type]) => `${name}: ${type}`).join(", ")} }`;

const literal = (value: number) => value.toFixed(1);

// Cloud water at a point, reconstructed from the simulated bands, in WGSL. The bands say where cloud is, how much
// of the sky it covers, how high it reaches, how thick it is optically, how icy, and what form it has; noise only
// places the elements a weather cell is too coarse to hold, at exactly the simulated cover. Positions are render
// space. The textures are function arguments, because WGSL binds them where three.js declares them:
//   bands   per band two layers, [cover, base, top, extinction per metre] and [ice, billow, breadth, streak]
//   shapes  heaped, smooth and cellular noise; details fine noise for wisps; both share `repeat`, a repeating
//           linear sampler
//   levels  for each cover step, the noise level above which that share of a layer lies, per character, read
//           through `repeat` at texel centres, so it filters between steps and never wraps
export const DensityWgsl = `
${CloudSkyStruct}

fn cloudRange(c: CloudSky, band: i32) -> vec4f {
  switch band { case 0: { return c.range0; } case 1: { return c.range1; } default: { return c.range2; } }
}

fn cloudDrift(c: CloudSky, band: i32) -> vec4f {
  switch band { case 0: { return c.drift0; } case 1: { return c.drift1; } default: { return c.drift2; } }
}

fn cloudWind(c: CloudSky, band: i32) -> vec2f {
  switch band { case 0: { return c.wind01.xy; } case 1: { return c.wind01.zw; } default: { return c.wind2; } }
}

fn smallRepeat(band: i32) -> f32 {
  switch band { case 0: { return ${literal(SmallRepeats[0])}; } case 1: { return ${literal(SmallRepeats[1])}; } default: { return ${literal(SmallRepeats[2])}; } }
}

// Weather cell coordinates of a render-space point, eased so that cells meet without creases.
fn cloudCell(c: CloudSky, p: vec3f) -> vec2f {
  let at = (p.xz * c.grid.z + c.grid.xy) * c.grid.w - 0.5;
  let part = fract(at);
  return (floor(at) + part * part * (3.0 - 2.0 * part) + 0.5) / c.grid.w;
}

// Smooth, cellular or heaped character, by how billowed the cloud is.
fn cloudForm(characters: vec3f, billow: f32) -> f32 {
  if (billow < 0.55) { return mix(characters.g, characters.b, billow / 0.55); }
  return mix(characters.b, characters.r, (billow - 0.55) / 0.45);
}

// The levels between two tabulated covers, filtered by the sampler: texel i is centred at (i + 0.5) / texels.
fn coverLevels(levels: texture_2d<f32>, filtered: sampler, cover: f32) -> vec3f {
  let at = (clamp(cover, 0.0, 1.0) * ${literal(CoverSteps)} + 0.5) / ${literal(CoverSteps + 1)};
  return textureSampleLevel(levels, filtered, vec2f(at, 0.5), 0.0).rgb;
}

// Extinction per metre of one band at a render-space point in x, its ice share in y. \`fine\` adds wisps.
fn bandCloud(c: CloudSky, bands: texture_2d_array<f32>, bandSampler: sampler, shapes: texture_3d<f32>, details: texture_3d<f32>,
    repeat: sampler, levels: texture_2d<f32>, band: i32, p: vec3f, cell: vec2f, fine: bool) -> vec2f {
  return bandCloudOf(c, bands, bandSampler, shapes, details, repeat, levels, band, cloudRange(c, band), smallRepeat(band),
    cloudWind(c, band), cloudDrift(c, band), p, cell, fine);
}

// bandCloud with the band's range, small repeat, wind and drift given, for a caller that holds them for many points.
fn bandCloudOf(c: CloudSky, bands: texture_2d_array<f32>, bandSampler: sampler, shapes: texture_3d<f32>, details: texture_3d<f32>,
    repeat: sampler, levels: texture_2d<f32>, band: i32, range: vec4f, size: f32, wind: vec2f, drift: vec4f, p: vec3f, cell: vec2f,
    fine: bool) -> vec2f {
  let y = p.y + c.datum;
  if (range.z <= 0.0 || y <= range.x || y >= range.y) { return vec2f(0.0); }
  let shape = textureSampleLevel(bands, bandSampler, cell, band * 2, 0.0);
  let h = (y - shape.y) / (shape.z - shape.y);
  if (shape.x < 0.01 || h <= 0.0 || h >= 1.0) { return vec2f(0.0); }
  let form = textureSampleLevel(bands, bandSampler, cell, band * 2 + 1, 0.0);
  let ice = form.x;
  let billow = form.y;
  let breadth = form.z;
  let streak = form.w;
  let tower = billow * breadth;

  let across = vec2f(-wind.y, wind.x);
  let plain = p.xz / size + drift.xy;
  let drawn = vec2f(dot(p.xz, wind) / (size * ${literal(StreakStretch)}), dot(p.xz, across) / size) + drift.zw;
  // Heaps are sampled stretched upwards: a dome then stands on the whole of its footprint and the cloud base cuts
  // it flat, as a condensation level does; a tower stays one column from its base to its anvil.
  let upright = mix(1.0, 0.4, billow);
  let broad = vec3f(p.x / ${literal(BroadRepeat)} + c.steer.x, y / ${literal(BroadRepeat)} * upright, p.z / ${literal(BroadRepeat)} + c.steer.y);
  let wide = textureSampleLevel(shapes, repeat, broad, 0.0).rgb;
  // The broad field pushes the small one about, which keeps its repeats from lining up into rows.
  let placed = mix(plain, drawn, streak) + (wide.rg - 0.5) * 0.5;
  let narrow = textureSampleLevel(shapes, repeat, vec3f(placed.x, y / size * upright, placed.y), 0.0).rgb;
  // Broad cloud keeps the small shapes as relief on its surface.
  let characters = mix(narrow, wide + (narrow - 0.5) * 0.3, breadth);

  // Small elements gather and thin out over tens of kilometres, as fields of cloud do; the mean stays the
  // simulated cover. Heaps narrow upwards into domes; a storm's top spreads into its anvil.
  var cover = shape.x * mix(1.0, 0.45 + 1.1 * wide.g, (1.0 - breadth) * (1.0 - shape.x));
  cover *= 1.0 - 0.75 * billow * (1.0 - breadth) * smoothstep(0.1, 1.0, h);
  if (band == 2) {
    cover = mix(cover, shape.x * (0.45 + 0.75 * smoothstep(0.3, 0.85, h)), tower);
  } else if (tower > 0.25 && h > 0.7) {
    // A tower nears the top of its band as wide as the band above lets it go on, so it either continues or domes.
    let above = textureSampleLevel(bands, bandSampler, cell, band * 2 + 2, 0.0).x * select(1.0, 0.45, band == 1);
    cover = mix(cover, min(cover, above), tower * smoothstep(0.7, 1.0, h));
  }
  let level = cloudForm(coverLevels(levels, repeat, cover), billow);
  var body = cloudForm(characters, billow) - level;
  if (body <= -0.12) { return vec2f(0.0, ice); }
  // Wisps move the cloud's surface in and out; without them (far away, or for light on its way) it sits at their
  // mean.
  var worn = 0.5;
  if (fine) {
    let wisps = textureSampleLevel(details, repeat, vec3f(placed.x, y / size, placed.y) * (size / ${literal(DetailRepeat)}), 0.0).rgb;
    worn = mix(1.0 - wisps.r, 1.0 - wisps.g, 0.35 + 0.4 * streak);
  }
  body -= (worn - 0.5) * mix(0.1, 0.22, max(billow, streak)) * mix(1.0, 0.35, breadth) * (1.0 - 0.6 * ice * (1.0 - streak));
  // Heaps have firm edges; sheets and fibres thin out gradually.
  var density = clamp(body / max(1.0 - level, 0.08) * mix(mix(1.4, 6.0, billow), 1.2, streak), 0.0, 1.0);
  if (density <= 0.0) { return vec2f(0.0, ice); }

  // Flat bases under heaps, soft faces on sheets. A tower runs through the faces between its bands.
  let foot = smoothstep(0.0, mix(0.3, 0.035, billow), h);
  let crown = 1.0 - smoothstep(mix(0.7, 0.85, billow) - 0.3 * ice * breadth, 1.0, h);
  density *= mix(foot, 1.0, select(0.0, tower, band > 0)) * mix(crown, 1.0, select(0.0, tower, band < 2));
  return vec2f(density * shape.w, ice);
}

// Extinction per metre of all bands together, without wisps: what light on its way through meets.
fn cloudShade(c: CloudSky, bands: texture_2d_array<f32>, bandSampler: sampler, shapes: texture_3d<f32>, details: texture_3d<f32>,
    repeat: sampler, levels: texture_2d<f32>, p: vec3f) -> f32 {
  let cell = cloudCell(c, p);
  return bandCloudOf(c, bands, bandSampler, shapes, details, repeat, levels, 0, c.range0, ${literal(SmallRepeats[0])}, c.wind01.xy, c.drift0, p, cell, false).x
    + bandCloudOf(c, bands, bandSampler, shapes, details, repeat, levels, 1, c.range1, ${literal(SmallRepeats[1])}, c.wind01.zw, c.drift1, p, cell, false).x
    + bandCloudOf(c, bands, bandSampler, shapes, details, repeat, levels, 2, c.range2, ${literal(SmallRepeats[2])}, c.wind2, c.drift2, p, cell, false).x;
}
`;
