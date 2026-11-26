import { wgslFn } from "three/tsl";

// Blends this frame's noisy march into the picture built up over the frames before. Each pixel looks up where its
// cloud was on screen last frame, taking the cloud to sit where its ray first meets a band; what it finds is kept
// only as far as this frame's neighbourhood allows, which is what stops trails. Texture coordinates run down the
// screen, as WebGPU's do.
export const resolveNode = wgslFn(`
fn cloudResolve(at: vec2f, direction: vec3f, marched: texture_2d<f32>, remembered: texture_2d<f32>, linear: sampler,
    pastClip: mat4x4f, eyeMoved: vec3f, eye: vec3f, datum: f32, range0: vec4f, range1: vec4f, range2: vec4f,
    texel: vec2f, persistence: f32) -> vec4f {
  let now = textureSampleLevel(marched, linear, at, 0.0);
  var reach = 40000.0;
  for (var band = 0; band < 3; band++) {
    var range = range0;
    if (band == 1) { range = range1; } else if (band == 2) { range = range2; }
    if (range.z <= 0.0) { continue; }
    let low = range.x - datum - eye.y;
    let high = range.y - datum - eye.y;
    var enter = 800.0;
    if (low > 0.0) { enter = low / max(direction.y, 0.0001); } else if (high < 0.0) { enter = high / min(direction.y, -0.0001); }
    if (enter > 0.0) { reach = min(reach, enter); }
  }
  let clip = pastClip * vec4f(direction * reach + eyeMoved, 1.0);
  let past = vec2f(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
  let found = clip.w > 0.0 && all(past > vec2f(0.0)) && all(past < vec2f(1.0));

  var least = now;
  var most = now;
  for (var y = -1; y <= 1; y++) {
    for (var x = -1; x <= 1; x++) {
      let beside = textureSampleLevel(marched, linear, at + vec2f(f32(x), f32(y)) * texel, 0.0);
      least = min(least, beside);
      most = max(most, beside);
    }
  }
  // One frame's march is noisy by design, so the bounds are generous: they are there to stop trails across the
  // sky, not to chase the noise.
  let slack = (most - least) * 0.75 + 0.06;
  let before = clamp(textureSampleLevel(remembered, linear, past, 0.0), least - slack, most + slack);
  return mix(now, before, select(0.0, persistence, found));
}
`);
