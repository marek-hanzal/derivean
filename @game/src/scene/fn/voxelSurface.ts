import { MeshStandardMaterial } from "three";

export function voxelSurface(voxelSize: number) {
  const material = new MeshStandardMaterial({ vertexColors: true, roughness: 1 });
  material.onBeforeCompile = (shader) => {
    shader.uniforms.voxelSize = { value: voxelSize };
    shader.vertexShader = shader.vertexShader.replace("#include <common>", `
      #include <common>
      attribute float surfaceRoughness;
      varying float vSurfaceRoughness;
      varying vec3 vVoxelPosition;
      varying vec3 vVoxelNormal;
    `).replace("#include <begin_vertex>", `
      #include <begin_vertex>
      vSurfaceRoughness = surfaceRoughness;
      vVoxelPosition = (modelMatrix * vec4(position, 1.0)).xyz;
      vVoxelNormal = normal;
    `);
    shader.fragmentShader = shader.fragmentShader.replace("#include <common>", `
      #include <common>
      uniform float voxelSize;
      varying float vSurfaceRoughness;
      varying vec3 vVoxelPosition;
      varying vec3 vVoxelNormal;
      float hash3( vec3 p ) {
        p = fract(p * 0.1031);
        p += dot(p, p.yzx + 33.33);
        return fract((p.x + p.y) * p.z);
      }
    `).replace("#include <color_fragment>", `
      #include <color_fragment>
      vec3 cell = (vVoxelPosition - vVoxelNormal * 0.001) / voxelSize;
      vec3 edge = min(fract(cell), 1.0 - fract(cell));
      edge += abs(vVoxelNormal);
      float grain = hash3(floor(cell));
      float broad = sin(vVoxelPosition.x * 0.87 + sin(vVoxelPosition.z * 0.93)) *
                    cos(vVoxelPosition.z * 1.1 + vVoxelPosition.y * 0.77);
      float detail = 1.0 - smoothstep(0.14, 0.57, max(max(fwidth(cell.x), fwidth(cell.y)), fwidth(cell.z)));
      float seam = smoothstep(0.015, 0.09, min(min(edge.x, edge.y), edge.z));
      diffuseColor.rgb *= 1.0 + broad * 0.065 + detail * ((grain - 0.5) * 0.12 - (1.0 - seam) * 0.095);
    `).replace("#include <roughnessmap_fragment>", `
      #include <roughnessmap_fragment>
      roughnessFactor *= vSurfaceRoughness;
    `);
  };
  return material;
}
