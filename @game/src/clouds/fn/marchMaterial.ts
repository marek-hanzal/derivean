import { ShaderMaterial, type IUniform } from "three";
import { skyRadiance } from "../../sky/constant/skyRadiance";
import { cloudDensity } from "../constant/density";
import { ClearStride, MarchSteps, StepGrowth, StepLengths, SunSteps, WispReach } from "../constant/march";
import { fullscreenVertex } from "./fullscreen";

const list = (values: readonly number[]) => values.map((value) => value.toFixed(1)).join(", ");

// One ray per pixel through the cloud bands in the order the ray meets them. Output: light scattered towards the
// eye in rgb and the share of what lies behind that still gets through in a.
export function marchMaterial(uniforms: Record<string, IUniform>) {
  return new ShaderMaterial({
    uniforms,
    depthTest: false,
    depthWrite: false,
    vertexShader: fullscreenVertex,
    fragmentShader: `
      uniform sampler2D sceneDepth;
      uniform mat4 cloudCamera;
      uniform vec2 cloudLens;
      uniform vec3 cloudEye;
      uniform float depthBase;
      uniform float farthest;
      uniform float hazeDensity;
      uniform float jitter;
      uniform vec3 sunLight;
      uniform vec3 skyAmbient;
      uniform vec3 groundAmbient;
      varying vec2 vUv;
      ${skyRadiance}
      ${cloudDensity}
      const vec3 stepLength = vec3(${list(StepLengths)});
      const vec3 sunStep = vec3(${list(SunSteps)});

      float lobe(float cosine, float g) {
        float g2 = g * g;
        return (1.0 - g2) / (12.566 * pow(1.0 + g2 - 2.0 * g * cosine, 1.5));
      }

      // Sunlight scattered at a point with \`shade\` of optical depth between it and the sun. Henyey-Greenstein
      // with a forward peak and a back lobe; each further order of scattering is broader, weaker, and less
      // shaded, which is what keeps thick cloud bright inside. Ice scatters less sharply back than droplets.
      float sunlit(float cosine, float shade, float ice) {
        float forward = mix(0.8, 0.7, ice);
        float back = mix(0.22, 0.1, ice);
        float sum = 0.0;
        float order = 1.0;
        for (int n = 0; n < 3; n++) {
          sum += order * mix(lobe(cosine, forward * order), lobe(cosine, -0.35 * order), back) * exp(-shade * order);
          order *= 0.5;
        }
        return sum;
      }

      void main() {
        vec3 viewRay = vec3((vUv * 2.0 - 1.0) * cloudLens, -1.0);
        float along = length(viewRay);
        vec3 direction = mat3(cloudCamera) * (viewRay / along);
        float depth = texture(sceneDepth, vUv).r;
        float limit = depth > 0.99999 ? farthest : min(farthest, (exp2(depth * depthBase) - 1.0) * along);
        float dither = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))) + jitter);
        float cosine = dot(direction, skySun);
        bool daylight = sunLight.r + sunLight.g + sunLight.b > 0.002;
        vec3 scattered = vec3(0.0);
        float through = 1.0;

        for (int k = 0; k < 3; k++) {
          int band = direction.y >= 0.0 ? k : 2 - k;
          vec4 range = cloudRange[band];
          if (range.z <= 0.0) continue;
          float low = range.x - cloudDatum - cloudEye.y;
          float high = range.y - cloudDatum - cloudEye.y;
          float enter = 0.0;
          float leave = limit;
          if (abs(direction.y) > 0.00002) {
            float a = low / direction.y;
            float b = high / direction.y;
            enter = max(min(a, b), 0.0);
            leave = min(max(a, b), limit);
          } else if (low > 0.0 || high < 0.0) continue;
          if (leave <= enter) continue;

          float t = enter + dither * max(stepLength[band], enter * ${StepGrowth});
          float strode = 0.0;
          for (int i = 0; i < ${MarchSteps}; i++) {
            if (t >= leave || through < 0.01) break;
            float pace = max(stepLength[band], t * ${StepGrowth});
            vec3 p = cloudEye + direction * t;
            float ice;
            float sigma = bandCloud(band, p, cloudCell(p), t < ${WispReach.toFixed(1)}, ice);
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
            // Towards the sun in growing steps, each sampled at a place within it that changes from pixel to
            // pixel and frame to frame, so the steps average out instead of drawing contours.
            float shade = 0.0;
            if (daylight) {
              float reach = sunStep[band];
              float gone = 0.0;
              float spot = fract(dither * 7.31 + 0.37);
              for (int j = 0; j < 5; j++) {
                shade += cloudShade(p + skySun * (gone + reach * spot)) * reach;
                gone += reach;
                reach *= 1.8;
              }
            }
            float height = clamp((p.y + cloudDatum - range.x) / (range.y - range.x), 0.0, 1.0);
            vec3 light = sunLight * sunlit(cosine, shade, ice) * 5.6
              + (skyAmbient * mix(0.4, 1.0, height) + groundAmbient * (1.0 - height) * 0.35) * 0.34;
            float absorbed = 1.0 - exp(-sigma * pace);
            float haze = 1.0 - exp(-pow(hazeDensity * t, 2.0));
            scattered += through * absorbed * mix(light, skyHorizon, haze);
            through *= 1.0 - absorbed;
            t += pace;
          }
        }
        gl_FragColor = vec4(scattered, through);
      }
    `,
  });
}
