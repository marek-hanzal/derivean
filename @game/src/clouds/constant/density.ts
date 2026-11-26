import { CoverSteps } from "../fn/noiseVolumes";
import { BroadRepeat, DetailRepeat, SmallRepeats, StreakStretch } from "./repeats";

const literal = (value: number) => value.toFixed(1);

// Cloud water at a point, reconstructed from the simulated bands. The bands say where cloud is, how much of the
// sky it covers, how high it reaches, how thick it is optically, how icy, and what form it has; noise only
// places the elements a weather cell is too coarse to hold, at exactly the simulated cover.
export const cloudDensity = `
  uniform highp sampler2DArray cloudBands;
  uniform highp sampler3D cloudShapes;
  uniform highp sampler3D cloudDetails;
  uniform sampler2D cloudLevels;
  uniform vec4 cloudGrid;
  uniform float cloudDatum;
  uniform vec4 cloudRange[3];
  uniform vec2 cloudSteer;
  uniform vec4 cloudDrift[3];
  uniform vec2 cloudWind[3];
  const vec3 smallRepeat = vec3(${SmallRepeats.map(literal).join(", ")});

  // Weather cell coordinates of a render-space point, eased so that cells meet without creases.
  vec2 cloudCell(vec3 p) {
    vec2 at = (p.xz * cloudGrid.z + cloudGrid.xy) * cloudGrid.w - 0.5;
    vec2 part = fract(at);
    return (floor(at) + part * part * (3.0 - 2.0 * part) + 0.5) / cloudGrid.w;
  }

  // Smooth, cellular or heaped character, by how billowed the cloud is.
  float cloudForm(vec3 characters, float billow) {
    return billow < 0.55 ? mix(characters.g, characters.b, billow / 0.55) : mix(characters.b, characters.r, (billow - 0.55) / 0.45);
  }

  // Extinction per metre of one band at a render-space point. \`fine\` adds wisps; \`ice\` is the ice share.
  float bandCloud(int band, vec3 p, vec2 cell, bool fine, out float ice) {
    ice = 0.0;
    vec4 range = cloudRange[band];
    float y = p.y + cloudDatum;
    if (range.z <= 0.0 || y <= range.x || y >= range.y) return 0.0;
    vec4 shape = texture(cloudBands, vec3(cell, float(band * 2)));
    float h = (y - shape.y) / (shape.z - shape.y);
    if (shape.x < 0.01 || h <= 0.0 || h >= 1.0) return 0.0;
    vec4 form = texture(cloudBands, vec3(cell, float(band * 2 + 1)));
    ice = form.x;
    float billow = form.y;
    float breadth = form.z;
    float streak = form.w;
    float tower = billow * breadth;

    float repeat = smallRepeat[band];
    vec2 wind = cloudWind[band];
    vec2 across = vec2(-wind.y, wind.x);
    vec2 plain = p.xz / repeat + cloudDrift[band].xy;
    vec2 drawn = vec2(dot(p.xz, wind) / (repeat * ${literal(StreakStretch)}), dot(p.xz, across) / repeat) + cloudDrift[band].zw;
    // Heaps are sampled stretched upwards: a dome then stands on the whole of its footprint and the cloud base
    // cuts it flat, as a condensation level does; a tower stays one column from its base to its anvil.
    float upright = mix(1.0, 0.4, billow);
    vec3 broad = vec3(p.x / ${literal(BroadRepeat)} + cloudSteer.x, y / ${literal(BroadRepeat)} * upright, p.z / ${literal(BroadRepeat)} + cloudSteer.y);
    vec3 wide = texture(cloudShapes, broad).rgb;
    // The broad field pushes the small one about, which keeps its repeats from lining up into rows.
    vec2 placed = mix(plain, drawn, streak) + (wide.rg - 0.5) * 0.5;
    vec3 small = vec3(placed.x, y / repeat * upright, placed.y);
    vec3 narrow = texture(cloudShapes, small).rgb;
    // Broad cloud keeps the small shapes as relief on its surface.
    vec3 characters = mix(narrow, wide + (narrow - 0.5) * 0.3, breadth);

    // Small elements gather and thin out over tens of kilometres, as fields of cloud do; the mean stays the
    // simulated cover. Heaps narrow upwards into domes; a storm's top spreads into its anvil.
    float cover = shape.x * mix(1.0, 0.45 + 1.1 * wide.g, (1.0 - breadth) * (1.0 - shape.x));
    cover *= 1.0 - 0.75 * billow * (1.0 - breadth) * smoothstep(0.1, 1.0, h);
    if (band == 2) cover = mix(cover, shape.x * (0.45 + 0.75 * smoothstep(0.3, 0.85, h)), tower);
    // A tower nears the top of its band as wide as the band above lets it go on, so it either continues or domes.
    else if (tower > 0.25 && h > 0.7) {
      float above = texture(cloudBands, vec3(cell, float(band * 2 + 2))).x * (band == 1 ? 0.45 : 1.0);
      cover = mix(cover, min(cover, above), tower * smoothstep(0.7, 1.0, h));
    }
    vec3 levels = texture(cloudLevels, vec2((clamp(cover, 0.0, 1.0) * ${literal(CoverSteps)} + 0.5) / ${literal(CoverSteps + 1)}, 0.5)).rgb;
    float level = cloudForm(levels, billow);
    float body = cloudForm(characters, billow) - level;
    if (body <= -0.12) return 0.0;
    // Wisps move the cloud's surface in and out; without them (far away, or for light on its way) it sits at
    // their mean.
    float worn = 0.5;
    if (fine) {
      vec3 wisps = texture(cloudDetails, vec3(placed.x, y / repeat, placed.y) * (repeat / ${literal(DetailRepeat)})).rgb;
      worn = mix(1.0 - wisps.r, 1.0 - wisps.g, 0.35 + 0.4 * streak);
    }
    body -= (worn - 0.5) * mix(0.1, 0.22, max(billow, streak)) * mix(1.0, 0.35, breadth) * (1.0 - 0.6 * ice * (1.0 - streak));
    // Heaps have firm edges; sheets and fibres thin out gradually.
    float density = clamp(body / max(1.0 - level, 0.08) * mix(mix(1.4, 6.0, billow), 1.2, streak), 0.0, 1.0);
    if (density <= 0.0) return 0.0;

    // Flat bases under heaps, soft faces on sheets. A tower runs through the faces between its bands.
    float foot = smoothstep(0.0, mix(0.3, 0.035, billow), h);
    float crown = 1.0 - smoothstep(mix(0.7, 0.85, billow) - 0.3 * ice * breadth, 1.0, h);
    density *= mix(foot, 1.0, band > 0 ? tower : 0.0) * mix(crown, 1.0, band < 2 ? tower : 0.0);
    return density * shape.w;
  }

  // Extinction per metre of all bands together, without wisps: what light on its way through meets.
  float cloudShade(vec3 p) {
    vec2 cell = cloudCell(p);
    float ice;
    return bandCloud(0, p, cell, false, ice) + bandCloud(1, p, cell, false, ice) + bandCloud(2, p, cell, false, ice);
  }
`;
