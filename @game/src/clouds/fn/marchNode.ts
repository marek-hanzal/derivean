import { wgsl, wgslFn } from "three/tsl";
import { DensityWgsl } from "../constant/density";
import { ClearStride, MarchSteps, StepGrowth, StepLengths, SunSteps, WispReach } from "../constant/march";
import { CloudParams, CloudSkyOf, CloudTextures } from "./cloudArgs";

const list = (values: readonly number[]) => values.map((value) => value.toFixed(1)).join(", ");

// One ray through the cloud bands in the order the ray meets them, from the eye for `limit` metres. Output: light
// scattered towards the eye in rgb and the share of what lies behind that still gets through in a. `dither`
// moves where the samples fall, from pixel to pixel and frame to frame.
export const marchNode = wgslFn(`
fn cloudMarch(direction: vec3f, limit: f32, dither: f32, eye: vec3f, sun: vec3f, sunLight: vec3f, skyAmbient: vec3f,
    groundAmbient: vec3f, horizon: vec3f, haze: f32, ${CloudParams}) -> vec4f {
  let c = ${CloudSkyOf};
  var stepLength = vec3f(${list(StepLengths)});
  var sunStep = vec3f(${list(SunSteps)});
  let cosine = dot(direction, sun);
  let daylight = sunLight.r + sunLight.g + sunLight.b > 0.002;
  var scattered = vec3f(0.0);
  var through = 1.0;

  for (var k = 0; k < 3; k++) {
    let band = select(2 - k, k, direction.y >= 0.0);
    let range = cloudRange(c, band);
    if (range.z <= 0.0) { continue; }
    let low = range.x - c.datum - eye.y;
    let high = range.y - c.datum - eye.y;
    var enter = 0.0;
    var leave = limit;
    if (abs(direction.y) > 0.00002) {
      let a = low / direction.y;
      let b = high / direction.y;
      enter = max(min(a, b), 0.0);
      leave = min(max(a, b), limit);
    } else if (low > 0.0 || high < 0.0) { continue; }
    if (leave <= enter) { continue; }

    var t = enter + dither * max(stepLength[band], enter * ${StepGrowth});
    var strode = 0.0;
    for (var i = 0; i < ${MarchSteps}; i++) {
      if (t >= leave || through < 0.01) { break; }
      let pace = max(stepLength[band], t * ${StepGrowth});
      let p = eye + direction * t;
      let found = bandCloud(c, ${CloudTextures}, band, p, cloudCell(c, p), t < ${WispReach.toFixed(1)});
      let sigma = found.x;
      if (sigma <= 0.0) {
        strode = pace * ${ClearStride};
        t += strode;
        continue;
      }
      // A long stride through clear air may have landed well inside: go back and come in at walking pace.
      if (strode > 0.0) {
        t -= strode - pace * 0.5;
        strode = 0.0;
        continue;
      }
      // Towards the sun in growing steps, each sampled at a place within it that changes from pixel to pixel and
      // frame to frame, so the steps average out instead of drawing contours.
      var shade = 0.0;
      if (daylight) {
        var reach = sunStep[band];
        var gone = 0.0;
        let spot = fract(dither * 7.31 + 0.37);
        for (var j = 0; j < 5; j++) {
          shade += cloudShade(c, ${CloudTextures}, p + sun * (gone + reach * spot)) * reach;
          gone += reach;
          reach *= 1.8;
        }
      }
      let height = clamp((p.y + c.datum - range.x) / (range.y - range.x), 0.0, 1.0);
      let light = sunLight * cloudSunlit(cosine, shade, found.y) * 5.6
        + (skyAmbient * mix(0.4, 1.0, height) + groundAmbient * (1.0 - height) * 0.35) * 0.34;
      let absorbed = 1.0 - exp(-sigma * pace);
      let hazed = 1.0 - exp(-pow(haze * t, 2.0));
      scattered += through * absorbed * mix(light, horizon, hazed);
      through *= 1.0 - absorbed;
      t += pace;
    }
  }
  return vec4f(scattered, through);
}
`, [wgsl(DensityWgsl), wgsl(`
fn cloudLobe(cosine: f32, g: f32) -> f32 {
  let g2 = g * g;
  return (1.0 - g2) / (12.566 * pow(1.0 + g2 - 2.0 * g * cosine, 1.5));
}

// Sunlight scattered at a point with \`shade\` of optical depth between it and the sun. Henyey-Greenstein with a
// forward peak and a back lobe; each further order of scattering is broader, weaker, and less shaded, which is
// what keeps thick cloud bright inside. Ice scatters less sharply back than droplets.
fn cloudSunlit(cosine: f32, shade: f32, ice: f32) -> f32 {
  let forward = mix(0.8, 0.7, ice);
  let back = mix(0.22, 0.1, ice);
  var sum = 0.0;
  var order = 1.0;
  for (var n = 0; n < 3; n++) {
    sum += order * mix(cloudLobe(cosine, forward * order), cloudLobe(cosine, -0.35 * order), back) * exp(-shade * order);
    order *= 0.5;
  }
  return sum;
}
`)]);
