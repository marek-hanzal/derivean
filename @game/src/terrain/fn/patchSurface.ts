import { MeshStandardMaterial, type Texture, type Vector2 } from "three";
import { cloudShadow } from "../../clouds/constant/shadow";
import { shadedLights } from "../../clouds/fn/shadedLights";
import type { CloudShade } from "../../clouds/type/CloudShade";
import { ground } from "../../scene/constant/ground";
import { GroundRepeat } from "../../scene/constant/grain";
import type { HandOff } from "../../scene/type/HandOff";
import type { SurfaceTone } from "../../scene/type/SurfaceTone";
import { airFade } from "../../sky/constant/airFade";
import type { Air } from "../../sky/type/Air";
import type { SkyLight } from "../../sky/type/SkyLight";
import { terrainShadow } from "../constant/shadow";
import type { TerrainShade } from "../type/TerrainShade";

// What every patch shares: the ground's look, the light and the shadows, the hand-off box, the palette.
export interface PatchLook {
  tone: SurfaceTone;
  handOff: HandOff;
  shade: CloudShade & TerrainShade;
  light: SkyLight;
  air: Air;
  groundPalette: { value: Texture };
}

// The far terrain's surface, one material per patch over one shared program. Its geometry is coarse; what makes
// it read as ground is the patch's shading raster: normals finer than the vertices, the surface material per
// texel, and how much sky each place sees; below a texel the ground's own unevenness tilts the light. Lighting,
// ground variation, shadows and aerial perspective are the voxel surface's, so both sides of the hand-off look
// alike. Patches reach `handOffOverlap` metres into the voxel box so the two never leave a gap between them.
// `shadeTexels` is the raster's cells along a side, `texelMetres` the width of one.
export function patchSurface(
  look: PatchLook, morphBand: { value: Vector2 }, shadeMap: Texture, shadeTexels: number, texelMetres: number,
) {
  const material = new MeshStandardMaterial({ roughness: 0.95, fog: false });
  material.onBeforeCompile = (shader) => {
    Object.assign(shader.uniforms, look.tone, look.handOff, look.shade, look.light, look.air, {
      morphBand, groundPalette: look.groundPalette,
      shadeMap: { value: shadeMap }, shadeTexels: { value: shadeTexels }, texelMetres: { value: texelMetres },
    });
    shader.vertexShader = shader.vertexShader.replace("#include <common>", `
      #include <common>
      attribute float parent;
      attribute float shore;
      attribute vec2 across;
      uniform vec2 morphBand;
      varying vec3 vPatchPosition;
      varying vec3 vAcross;
    `).replace("#include <begin_vertex>", `
      vec3 transformed = vec3(position);
      vPatchPosition = (modelMatrix * vec4(position, 1.0)).xyz;
      float blend = clamp((distance(vPatchPosition, cameraPosition) - morphBand.x) / (morphBand.y - morphBand.x), 0.0, 1.0);
      transformed.y = mix(position.y, parent, blend);
      vPatchPosition.y += transformed.y - position.y;
      vAcross = vec3(across, shore);
    `);
    shader.fragmentShader = shader.fragmentShader.replace("#include <common>", `
      #include <common>
      uniform vec3 boxMin;
      uniform vec3 boxMax;
      uniform sampler2D shadeMap;
      uniform sampler2D groundPalette;
      uniform float shadeTexels;
      uniform float texelMetres;
      varying vec3 vPatchPosition;
      varying vec3 vAcross;
      const float handOffOverlap = 0.5;
      ${ground}
      ${cloudShadow}
      ${terrainShadow}
      ${airFade}
    `).replace("#include <clipping_planes_fragment>", `
      if (all(greaterThan(vPatchPosition, boxMin + handOffOverlap)) && all(lessThan(vPatchPosition, boxMax - handOffOverlap))) discard;
      #include <clipping_planes_fragment>
    `).replace("#include <color_fragment>", `
      #include <color_fragment>
      // The nine raster texels round this place, blended by hand, twice. Normals and openness blend as numbers
      // between the nearest four, but only with texels that face much as the nearest one does: across a crease
      // each side keeps its own slope. Materials blend by their colours with weights that give every pair of
      // neighbouring texels equal say, which is what turns a dithered mix of two materials into the mixture it
      // stands for instead of a chequerboard.
      vec2 place = clamp(vAcross.xy, 0.0, 1.0) * shadeTexels;
      ivec2 middle = ivec2(floor(place + 0.5));
      vec2 off = place - vec2(middle);
      mat3 exact = mat3(vec3(max(-off.x, 0.0), 1.0 - abs(off.x), max(off.x, 0.0)), vec3(max(-off.y, 0.0), 1.0 - abs(off.y), max(off.y, 0.0)), vec3(0.0));
      vec4 raster = vec4(0.0);
      vec2 facing = (texelFetch(shadeMap, middle, 0).rg * 255.0 - 128.0) / 127.0;
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          vec4 texel = texelFetch(shadeMap, clamp(middle + ivec2(i - 1, j - 1), ivec2(0), ivec2(int(shadeTexels))), 0);
          vec2 other = (texel.rg * 255.0 - 128.0) / 127.0;
          float alike = dot(facing, other) + sqrt(max((1.0 - dot(facing, facing)) * (1.0 - dot(other, other)), 0.0));
          raster += vec4(texel.rg, texel.a, 1.0) * (exact[0][i] * exact[1][j] * smoothstep(0.82, 0.96, alike));
        }
      }
      // A trick: where one material ends and the next begins is known to a texel, so the border is drawn
      // wandering by up to a texel instead of along the raster's steps. Which material lies where stays the
      // raster's.
      vec3 swayAt = (vPatchPosition + groundAnchor) / (texelMetres * 4.0);
      vec3 swayRepeat = vec3(${GroundRepeat.toFixed(1)} / (texelMetres * 4.0));
      vec2 sway = vec2(groundNoise(swayAt * vec3(1.0, 0.0, 1.0), swayRepeat), groundNoise(swayAt * vec3(1.0, 0.0, 1.0) + 31.7, swayRepeat)) - 0.5;
      vec2 swayed = clamp(place + sway * 2.0, 0.0, shadeTexels);
      ivec2 centre = ivec2(floor(swayed + 0.5));
      vec2 shift = swayed - vec2(centre);
      mat3 evenly = mat3(vec3(0.25 - shift.x * 0.5, 0.5, 0.25 + shift.x * 0.5), vec3(0.25 - shift.y * 0.5, 0.5, 0.25 + shift.y * 0.5), vec3(0.0));
      vec4 surface = vec4(0.0);
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          float id = texelFetch(shadeMap, clamp(centre + ivec2(i - 1, j - 1), ivec2(0), ivec2(int(shadeTexels))), 0).b;
          surface += texelFetch(groundPalette, ivec2(int(id * 255.0 + 0.5), 0), 0) * (evenly[0][i] * evenly[1][j]);
        }
      }
      raster /= raster.a;
      vec2 lean = (raster.rg * 255.0 - 128.0) / 127.0;
      float up = sqrt(max(1.0 - dot(lean, lean), 0.0));
      float wet = vAcross.z;
      // A trick, not light transport: ground that sees less sky is darkened, which is what lets gullies and the
      // feet of walls separate from open slopes under a high sun.
      float open = mix(0.42, 1.0, raster.b);
      float rise;
      vec3 varied = groundColor(surface.rgb, vPatchPosition, surface.a, vViewPosition.z, rise) * mix(1.0, 0.55, wet) * open;
      diffuseColor.rgb *= mix(varied, plainTone * mix(0.7, 1.0, raster.b), plain);
      float sunShare = mix(cloudShadow(vPatchPosition, skySun), 1.0, plain) * terrainShadow(vPatchPosition, skySun, vPatchPosition - cameraPosition);
    `).replace("#include <normal_fragment_begin>", `
      #include <normal_fragment_begin>
      normal = groundBump(normalize(mat3(viewMatrix) * vec3(lean.x, up, lean.y)), -vViewPosition, rise * (1.0 - plain));
    `).replace("#include <lights_fragment_begin>", shadedLights()).replace("#include <fog_fragment>", `
      gl_FragColor.rgb = airFade(gl_FragColor.rgb, vPatchPosition - cameraPosition);
    `);
  };
  return material;
}
