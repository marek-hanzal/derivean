import { DoubleSide, type Group, Mesh, MeshStandardMaterial, type Color } from "three";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { voxelGeometry } from "../../scene/fn/voxelGeometry";

export function createWaterScene(content: Group, world: WorldInfo, palette: readonly Color[]) {
  const material = new MeshStandardMaterial({
    vertexColors: true,
    roughness: 0.27,
    metalness: 0.06,
    opacity: 0.76,
    transparent: true,
    side: DoubleSide,
    forceSinglePass: true,
    emissive: "#0a2938",
  });
  const meshes = new Map<string, Mesh>();

  function remove(id: string) {
    const part = meshes.get(id);
    if (!part) return;
    content.remove(part);
    part.geometry.dispose();
    meshes.delete(id);
  }

  function apply(frame: Frame) {
    for (const key of frame.unloaded) remove(key.join(","));
    for (const { key, origin, faces } of frame.waterMeshes) {
      const id = key.join(",");
      remove(id);
      if (!faces.length) continue;
      const geometry = voxelGeometry(faces, world, palette, world.waterUnitsPerVoxel);
      const mesh = new Mesh(geometry, material);
      mesh.position.set(...origin);
      mesh.matrixAutoUpdate = false;
      mesh.updateMatrix();
      mesh.receiveShadow = true;
      meshes.set(id, mesh);
      content.add(mesh);
    }
  }

  return {
    apply,
    dispose() {
      for (const id of [...meshes.keys()]) remove(id);
      material.dispose();
    },
  };
}
