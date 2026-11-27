import type { Group, Material } from "three/webgpu";
import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import { FreeLevel } from "../../cubes/constant/levels";
import { createFaceBatch, type FaceBatch } from "../../cubes/service/FaceBatch";

// The engine's meshes of voxel water, one per chunk, in water units of a voxel. A unit is too small for a chunk
// to be counted from anywhere but its own origin, so each chunk is a batch of its own. A chunk's origin follows from
// its key alone (chunkOrigin in @engine/src/game/fn/WorldProjection.cpp), and flowing water sends a chunk's mesh
// again tick after tick: the new mesh is written into the chunk's batch in place, which keeps its buffers.
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
        if (!faces.length) {
          remove(id);
          continue;
        }
        let batch = chunks.get(id);
        if (!batch) {
          batch = createFaceBatch(content, surface, FreeLevel, edge, world.chunkMeters / edge);
          chunks.set(id, batch);
        }
        batch.fill(id, faces, world.faceWords, origin);
        if (batch.mesh) batch.mesh.renderOrder = 2;
      }
    },
    dispose() {
      for (const id of [...chunks.keys()]) remove(id);
    },
  };
}
