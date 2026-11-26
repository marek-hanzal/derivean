import type { Group, Material } from "three/webgpu";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { FreeLevel } from "../../cubes/constant/levels";
import { createFaceBatch, type FaceBatch } from "../../cubes/service/FaceBatch";

// The engine's meshes of voxel water, one per chunk, in water units of a voxel. A unit is too small for a chunk
// to be counted from anywhere but its own origin, so each chunk is a batch of its own.
export function createVoxelWater(content: Group, world: WorldInfo, surface: Material) {
  const edge = world.voxelSize / world.waterUnitsPerVoxel;
  const chunks = new Map<string, FaceBatch>();

  function remove(id: string) {
    chunks.get(id)?.dispose();
    chunks.delete(id);
  }

  return {
    apply(frame: Frame) {
      for (const key of frame.unloaded) remove(key.join(","));
      for (const { key, origin, faces } of frame.waterMeshes) {
        const id = key.join(",");
        remove(id);
        if (!faces.length) continue;
        const chunk = createFaceBatch(content, surface, FreeLevel, edge, world.chunkMeters / edge);
        chunk.fill(id, faces, world.faceWords, origin);
        if (chunk.mesh) chunk.mesh.renderOrder = 2;
        chunks.set(id, chunk);
      }
    },
    dispose() {
      for (const id of [...chunks.keys()]) remove(id);
    },
  };
}
