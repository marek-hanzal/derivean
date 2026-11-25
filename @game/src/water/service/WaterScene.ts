import { DoubleSide, Mesh, MeshStandardMaterial, type Color, type Scene } from "three";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { voxelGeometry } from "../../scene/fn/voxelGeometry";

export function createWaterScene(scene: Scene, world: WorldInfo, palette: readonly Color[]) {
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
    scene.remove(part);
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
      mesh.receiveShadow = true;
      meshes.set(id, mesh);
      scene.add(mesh);
    }
  }

  return { apply };
}
