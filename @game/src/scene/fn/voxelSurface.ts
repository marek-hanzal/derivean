import { MeshStandardMaterial, ShaderChunk, Vector2, type Vector3 } from "three";
import { cloudShadow } from "../../clouds/constant/shadow";
import { shadedLights } from "../../clouds/fn/shadedLights";
import type { CloudShade } from "../../clouds/type/CloudShade";
import { airFade } from "../../sky/constant/airFade";
import type { Air } from "../../sky/type/Air";
import { MeanShading, ShadowFade } from "../../sky/constant/shadow";
import type { SkyLight } from "../../sky/type/SkyLight";
import { terrainShadow } from "../../terrain/constant/shadow";
import type { TerrainShade } from "../../terrain/type/TerrainShade";
import { ground } from "../constant/ground";
import type { HandOff } from "../type/HandOff";
import type { Occlusion } from "../type/Occlusion";
import type { SurfaceTone } from "../type/SurfaceTone";

const shadowReturn = "return mix( 1.0, shadow, shadowIntensity );";

// three.js cuts shadows off at the edge of the shadow frustum. This variant of its lookups lets them thin out over
// the outer part instead, so the limit of the near shadows never shows as a line on open ground.
function fadingShadows() {
  const chunk = ShaderChunk.shadowmap_pars_fragment;
  if (!chunk.includes(shadowReturn)) throw new Error("three.js shadow lookup changed; revisit the shadow edge fade");
  return `
    uniform float shadowFade;
    // Shadow coordinates span 0…1 across the frustum, so its half extent is 0.5.
    float shadowEdge(vec2 coord) {
      vec2 inside = min(coord, 1.0 - coord);
      return clamp(min(inside.x, inside.y) / max(shadowFade * 0.5, 0.00001), 0.0, 1.0);
    }
    ${chunk.replaceAll(shadowReturn, "return mix( 1.0, shadow, shadowIntensity * shadowEdge( shadowCoord.xy ) );")}
  `;
}

// The look of voxels: the material's colour varied as ground is, each voxel a shade of its own with a seam round
// it, shaded by clouds and distance like the far terrain. Terrain chunks pass a `place`: their creases are
// darkened from the occlusion volume, and fragments outside the voxel box's footprint are left to the far terrain.
export function voxelSurface(
  tone: SurfaceTone, shade: CloudShade & TerrainShade, light: SkyLight, air: Air,
  place?: HandOff & Occlusion,
) {
  const material = new MeshStandardMaterial({ vertexColors: true, roughness: 1, fog: false });
  if (place) material.defines = { ...material.defines, HAND_OFF: "" };
  const uniforms = {
    ...tone,
    ...shade,
    ...light,
    ...place,
    ...air,
    shadowFade: { value: ShadowFade },
    meanRange: { value: new Vector2(...MeanShading) },
  };
  material.onBeforeCompile = (shader) => {
    Object.assign(shader.uniforms, uniforms);
    shader.vertexShader = shader.vertexShader.replace("#include <common>", `
      #include <common>
      attribute float surfaceRoughness;
      attribute float loose;
      attribute vec3 meanNormal;
      varying float vSurfaceRoughness;
      varying float vLoose;
      varying vec3 vVoxelPosition;
      varying vec3 vVoxelNormal;
      varying vec3 vMeanNormal;
    `).replace("#include <begin_vertex>", `
      #include <begin_vertex>
      vSurfaceRoughness = surfaceRoughness;
      vLoose = loose;
      vVoxelPosition = (modelMatrix * vec4(position, 1.0)).xyz;
      vVoxelNormal = normal;
      vMeanNormal = normalMatrix * meanNormal;
    `);
    shader.fragmentShader = shader.fragmentShader.replace("#include <common>", `
      #include <common>
      uniform float voxelSize;
      uniform float grainCells;
      uniform vec2 meanRange;
      uniform vec3 boxMin;
      uniform vec3 boxMax;
      uniform highp sampler3D voxelOcclusion;
      uniform vec4 occlusionPlace;
      uniform vec2 occlusionReach;
      varying float vSurfaceRoughness;
      varying float vLoose;
      varying vec3 vVoxelPosition;
      varying vec3 vVoxelNormal;
      varying vec3 vMeanNormal;
      ${ground}
      ${cloudShadow}
      ${terrainShadow}
      ${airFade}
    `).replace("#include <shadowmap_pars_fragment>", fadingShadows()).replace("#include <clipping_planes_fragment>", `
      #ifdef HAND_OFF
        if (any(lessThan(vVoxelPosition.xz, boxMin.xz)) || any(greaterThan(vVoxelPosition.xz, boxMax.xz))) discard;
      #endif
      #include <clipping_planes_fragment>
    `).replace("#include <color_fragment>", `
      #include <color_fragment>
      vec3 cell = (vVoxelPosition - vVoxelNormal * 0.001) / voxelSize;
      vec3 edge = min(fract(cell), 1.0 - fract(cell));
      edge += abs(vVoxelNormal);
      float grain = groundHash(mod(floor(cell), grainCells));
      float pixel = max(max(fwidth(cell.x), fwidth(cell.y)), fwidth(cell.z));
      float detail = 1.0 - smoothstep(0.14, 0.57, pixel);
      float seam = smoothstep(0.015, 0.09, min(min(edge.x, edge.y), edge.z));
      // What a material is made of shows within a voxel as specks a quarter and a twelfth of it across, with the
      // odd pale stone and dark pit among them: plain in loose grains, faint on a face of rock. Each size is
      // gone before it is smaller than a pixel.
      float coarse = groundHash(mod(floor(cell * 4.0), grainCells * 4.0));
      float fines = groundHash(mod(floor(cell * 12.0) + 7.0, grainCells * 12.0));
      float specks = ((coarse - 0.5) * 0.6 + step(0.94, coarse) * 0.5 - step(coarse, 0.07) * 0.5) * (1.0 - smoothstep(0.04, 0.12, pixel))
        + (fines - 0.5) * (1.0 - smoothstep(0.015, 0.04, pixel));
      // A voxel is one piece of ground: it takes the ground's variation at its middle, a tone of its own, and a
      // faint seam round its faces.
      float tone = 1.0 + detail * ((grain - 0.5) * mix(0.08, 0.14, vLoose) - (1.0 - seam) * 0.04) + specks * mix(0.07, 0.2, vLoose);
      vec3 middle = (floor(cell) + 0.5) * voxelSize;
      // Half a voxel off the face lies the middle of the open voxel in front of it; solid neighbours of that voxel
      // show in the filtered volume as a crease, a second sample further out as the corner of a deeper hollow.
      float crease = 0.0;
      #ifdef HAND_OFF
        float within = occlusionReach.y * (1.0 - smoothstep(occlusionReach.x * 0.7, occlusionReach.x, length(vVoxelPosition - cameraPosition)));
        if (within > 0.0) {
          vec3 inVolume = vVoxelPosition * occlusionPlace.w + occlusionPlace.xyz;
          float beside = texture(voxelOcclusion, inVolume + vVoxelNormal * (0.5 * voxelSize * occlusionPlace.w)).r;
          float beyond = texture(voxelOcclusion, inVolume + vVoxelNormal * (1.5 * voxelSize * occlusionPlace.w)).r;
          crease = within * min(beside * 1.1 + beyond * 0.45, 1.0);
        }
      #endif
      float rise;
      vec3 varied = groundColor(diffuseColor.rgb, middle, vLoose, vViewPosition.z, rise) * tone * (1.0 - 0.6 * crease);
      diffuseColor.rgb = mix(varied, plainTone, plain);
      float sunShare = mix(cloudShadow(vVoxelPosition, skySun), 1.0, plain) * terrainShadow(vVoxelPosition, skySun, vVoxelPosition - cameraPosition);
    `).replace("#include <lights_fragment_begin>", shadedLights()).replace("#include <roughnessmap_fragment>", `
      #include <roughnessmap_fragment>
      roughnessFactor *= vSurfaceRoughness;
    `).replace("#include <normal_fragment_begin>", `
      #include <normal_fragment_begin>
      normal = normalize(mix(normal, normalize(vMeanNormal), smoothstep(meanRange.x, meanRange.y, vViewPosition.z) * (1.0 - plain)));
    `).replace("#include <fog_fragment>", `
      gl_FragColor.rgb = airFade(gl_FragColor.rgb, vVoxelPosition - cameraPosition);
    `);
  };
  return material;
}
