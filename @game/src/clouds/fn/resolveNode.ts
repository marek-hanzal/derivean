import { wgslFn } from "three/tsl";

// Blends this frame's noisy march into the picture built up over the frames before. Each pixel looks up where its
// cloud was on screen last frame, taking the cloud to sit where its ray first meets a band; what it finds is kept
// only as far as this frame's neighbourhood allows, which is what stops trails. Turning is undone exactly; the eye's
// own movement only as well as that guess of the cloud's distance holds, so the further the eye's movement shifts
// the picture, the closer the past is held to this frame: in fast flight it would otherwise leave a cloud's edge
// behind in steps. How much of it is kept stays the same, so the march's noise is averaged as well as at rest.
// Texture coordinates run down the screen, as WebGPU's do. `pastMoved` is the eye's movement through the last
// frame's lens, the same for every pixel.
export const resolveNode = wgslFn(`
fn cloudResolve(at: vec2f, direction: vec3f, marched: texture_2d<f32>, remembered: texture_2d<f32>, linear: sampler,
    pastClip: mat4x4f, pastMoved: vec4f, eye: vec3f, datum: f32, range0: vec4f, range1: vec4f, range2: vec4f,
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
  let turned = pastClip * vec4f(direction * reach, 1.0);
  let clip = turned + pastMoved;
  let past = vec2f(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
  let found = clip.w > 0.0 && all(past > vec2f(0.0)) && all(past < vec2f(1.0));
  // Nothing of the past to keep: a first frame, or a cloud that was off the last frame's screen.
  if (!found || persistence <= 0.0) { return now; }
  let unmoved = vec2f(turned.x, -turned.y) / max(turned.w, 1e-6) * 0.5;
  // Behind the last frame's eye the shift has no meaning; it then counts as large.
  let parallax = select(1000.0, length((past - unmoved - vec2f(0.5)) / texel), clip.w > 0.0 && turned.w > 0.0);
  // 0 while the eye's movement shifts the picture by less than a texel, 1 from four texels on.
  let fast = smoothstep(1.0, 4.0, parallax);

  var least = now;
  var most = now;
  for (var y = -1; y <= 1; y++) {
    for (var x = -1; x <= 1; x++) {
      if (x == 0 && y == 0) { continue; }
      let beside = textureSampleLevel(marched, linear, at + vec2f(f32(x), f32(y)) * texel, 0.0);
      least = min(least, beside);
      most = max(most, beside);
    }
  }
  // One frame's march is noisy by design, so the bounds are generous: they are there to stop trails across the
  // sky, not to chase the noise.
  let slack = (most - least) * mix(0.75, 0.25, fast) + mix(0.06, 0.01, fast);
  let before = clamp(textureSampleLevel(remembered, linear, past, 0.0), least - slack, most + slack);
  return mix(now, before, persistence);
}
`);
