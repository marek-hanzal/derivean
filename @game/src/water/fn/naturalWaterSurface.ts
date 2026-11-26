import { DoubleSide, ShaderMaterial } from "three";
import { cloudShadow } from "../../clouds/constant/shadow";
import type { CloudShade } from "../../clouds/type/CloudShade";
import { airFade } from "../../sky/constant/airFade";
import type { Air } from "../../sky/type/Air";
import type { SkyLight } from "../../sky/type/SkyLight";

// Seas and lakes of the far terrain: tint and opacity grow with depth, so the surface dissolves into the shore
// where the terrain rises through it. Looked into it shows its depth, looked along it mirrors the sky, and far
// away the air takes it as it takes the ground.
export function naturalWaterSurface(light: SkyLight, air: Air, shade: CloudShade, plain: { value: number }) {
  return new ShaderMaterial({
    transparent: true,
    depthWrite: false,
    side: DoubleSide,
    uniforms: { ...light, ...air, ...shade, plain },
    vertexShader: `
      attribute float depth;
      varying float vDepth;
      varying vec3 vFromEye;
      #include <common>
      #include <logdepthbuf_pars_vertex>
      void main() {
        vDepth = depth;
        vec4 placed = modelMatrix * vec4(position, 1.0);
        vFromEye = placed.xyz - cameraPosition;
        vec4 mvPosition = viewMatrix * placed;
        gl_Position = projectionMatrix * mvPosition;
        #include <logdepthbuf_vertex>
      }
    `,
    fragmentShader: `
      uniform vec3 sunLight;
      uniform vec3 skyAmbient;
      uniform float plain;
      ${airFade}
      ${cloudShadow}
      varying float vDepth;
      varying vec3 vFromEye;
      #include <common>
      #include <logdepthbuf_pars_fragment>
      const vec3 shallow = vec3(0.07, 0.2, 0.2);
      const vec3 deep = vec3(0.008, 0.04, 0.075);
      void main() {
        #include <logdepthbuf_fragment>
        float depth = max(vDepth, 0.0);
        vec3 toEye = normalize(-vFromEye);
        float facing = abs(toEye.y);
        // Schlick's reflectance of water: a mirror of the sky at grazing angles, clear when looked into.
        float fresnel = min(0.02 + 0.98 * pow(1.0 - facing, 5.0), 0.85);
        vec3 mirror = vec3(-toEye.x, facing, -toEye.z);
        vec3 sunlight = sunLight * cloudShadow(vFromEye + cameraPosition, skySun);
        vec3 body = mix(shallow, deep, 1.0 - exp(-depth / 14.0)) * (skyAmbient * 0.55 + sunlight * max(skySun.y, 0.0) * 0.4);
        vec3 mirrored = airLight(mirror, length(vFromEye));
        vec3 glint = sunlight * pow(max(dot(mirror, skySun), 0.0), 180.0) * 0.4;
        vec3 natural = mix(body, mirrored, fresnel) + glint;
        vec3 relief = mix(vec3(0.2, 0.36, 0.52), vec3(0.07, 0.15, 0.3), 1.0 - exp(-depth / 14.0));
        // A thin paler line marks the shore: where the depth runs out, a pixel or two wide whatever the bed's
        // slope, and nowhere on water that is merely shallow.
        float edge = max(fwidth(vDepth), 1e-4);
        float line = (1.0 - smoothstep(edge, edge * 2.5, vDepth)) * step(0.0, vDepth) * smoothstep(0.01, 0.05, edge);
        natural = mix(natural, natural * 1.6 + 0.03, line * 0.6);
        float cover = max(1.0 - exp(-depth / 0.9), line * 0.5);
        gl_FragColor = vec4(airFade(mix(natural, relief, plain), vFromEye), cover * mix(0.8, 1.0, max(fresnel, plain)));
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }
    `,
  });
}
