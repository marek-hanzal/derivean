import { ShaderMaterial, UniformsLib, UniformsUtils, Vector3 } from "three";
import type { SkyLight } from "../../sky/type/SkyLight";
import { FallBox, type Fall } from "../constant/fall";

// Drops as camera-facing streaks along their velocity. Each lives at a fixed place in a cube of world space that
// repeats, falls through it, and is drawn where it wraps nearest the eye, so the rain stays put when the eye
// moves instead of following it.
export function fallMaterial(fall: Fall, light: SkyLight) {
  return new ShaderMaterial({
    transparent: true,
    depthWrite: false,
    fog: true,
    uniforms: {
      ...UniformsUtils.clone(UniformsLib.fog),
      skyAmbient: light.skyAmbient,
      sunLight: light.sunLight,
      // Where the eye is in the cube, and how far the drops have fallen through it, both in cubes.
      eyeInBox: { value: new Vector3() },
      fallen: { value: new Vector3() },
      velocity: { value: new Vector3(0, -fall.speed, 0) },
      seconds: { value: 0 },
    },
    vertexShader: `
      attribute vec3 seed;
      uniform vec3 eyeInBox;
      uniform vec3 fallen;
      uniform vec3 velocity;
      uniform float seconds;
      varying float vFade;
      #include <common>
      #include <fog_pars_vertex>
      #include <logdepthbuf_pars_vertex>
      void main() {
        vec3 centre = (fract(seed + fallen - eyeInBox) - 0.5) * ${FallBox.toFixed(1)};
        centre.xz += sin(seconds * 1.3 + seed.yx * 40.0) * ${fall.sway.toFixed(3)};
        // A drop is as long as it travels in its exposure and never shorter than it is wide.
        vec3 trail = velocity * ${fall.trail.toFixed(4)};
        trail *= max(1.0, ${fall.width.toFixed(4)} / length(trail));
        vec3 side = normalize(cross(trail, -centre)) * ${fall.width.toFixed(4)};
        vec3 placed = centre + trail * (position.y - 0.5) + side * position.x;
        float range = length(centre);
        vFade = (1.0 - smoothstep(${(FallBox * 0.3).toFixed(1)}, ${(FallBox * 0.5).toFixed(1)}, range)) * smoothstep(0.4, 1.6, range);
        vec4 mvPosition = modelViewMatrix * vec4(placed, 1.0);
        gl_Position = projectionMatrix * mvPosition;
        #include <logdepthbuf_vertex>
        #include <fog_vertex>
      }
    `,
    fragmentShader: `
      uniform vec3 skyAmbient;
      uniform vec3 sunLight;
      varying float vFade;
      #include <common>
      #include <fog_pars_fragment>
      #include <logdepthbuf_pars_fragment>
      void main() {
        #include <logdepthbuf_fragment>
        gl_FragColor = vec4(skyAmbient * 0.4 + sunLight * 0.06, vFade * ${fall.opacity.toFixed(2)});
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
        #include <fog_fragment>
      }
    `,
  });
}
