// Tileable noise for the structure below a weather cell, in WGSL. Every function takes a point in repeats (0…1 on
// each axis) and a period in features per repeat; integer cells wrap at the period, so the volume tiles.
export const NoiseWgsl = `
fn pcg3(input: vec3u) -> vec3u {
  var v = input * 1664525u + 1013904223u;
  v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
  v ^= v >> vec3u(16u);
  v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
  return v;
}

fn noiseHash(cell: vec3f, period: f32) -> vec3f {
  let wrapped = cell - period * floor(cell / period);
  return vec3f(pcg3(vec3u(vec3i(wrapped)))) / 4294967295.0;
}

fn worley(p: vec3f, period: f32) -> f32 {
  let cell = floor(p * period);
  let inside = fract(p * period);
  var nearest = 1.0;
  for (var z = -1; z <= 1; z++) {
    for (var y = -1; y <= 1; y++) {
      for (var x = -1; x <= 1; x++) {
        let offset = vec3f(f32(x), f32(y), f32(z));
        let point = noiseHash(cell + offset, period) + offset - inside;
        nearest = min(nearest, dot(point, point));
      }
    }
  }
  return 1.0 - min(sqrt(nearest), 1.0);
}

fn perlin(p: vec3f, period: f32) -> f32 {
  let cell = floor(p * period);
  let inside = fract(p * period);
  let ease = inside * inside * inside * (inside * (inside * 6.0 - 15.0) + 10.0);
  var sum = 0.0;
  for (var z = 0; z <= 1; z++) {
    for (var y = 0; y <= 1; y++) {
      for (var x = 0; x <= 1; x++) {
        let corner = vec3f(f32(x), f32(y), f32(z));
        let slope = normalize(noiseHash(cell + corner, period) * 2.0 - 1.0);
        let weight = mix(1.0 - ease, ease, corner);
        sum += weight.x * weight.y * weight.z * dot(slope, inside - corner);
      }
    }
  }
  return sum;
}

fn worleys(p: vec3f, period: f32) -> f32 {
  return worley(p, period) * 0.625 + worley(p, period * 2.0) * 0.25 + worley(p, period * 4.0) * 0.125;
}

fn perlins(p: vec3f, period: f32) -> f32 {
  let sum = perlin(p, period) + perlin(p, period * 2.0) * 0.5 + perlin(p, period * 4.0) * 0.25;
  return clamp(sum * 0.85 + 0.5, 0.0, 1.0);
}

// Three characters at four features a repeat: r heaped (Perlin swollen and then bitten by Worley, which gives
// domes), g smooth (Perlin), b cellular (Worley); a is fine Worley.
fn shapeNoise(p: vec3f) -> vec4f {
  let soft = perlins(p, 4.0);
  let cells = worleys(p, 4.0);
  let swollen = cells + soft * (1.0 - cells);
  let bite = (1.0 - worleys(p, 8.0)) * 0.5;
  let heaped = clamp((swollen - bite) / (1.0 - bite), 0.0, 1.0);
  return vec4f(heaped, soft, cells, worleys(p, 12.0));
}

// Fine Worley at two sizes for wisps, and smooth Perlin.
fn detailNoise(p: vec3f) -> vec4f {
  return vec4f(worleys(p, 4.0), worleys(p, 8.0), perlins(p, 4.0), 1.0);
}
`;
