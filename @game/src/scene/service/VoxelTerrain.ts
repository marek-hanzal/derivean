import { BufferAttribute, type Color, type Group, type Material, Mesh, Vector3 } from "three";
import type { Frame, Point, WorldInfo } from "../../../contract/GameBridge";
import { looseness } from "../fn/looseness";
import { meanNormals } from "../fn/meanNormals";
import { voxelGeometry } from "../fn/voxelGeometry";

// Streamed terrain chunks and detached rigid pieces. Vertices stay relative to the chunk or the piece; the mesh
// position carries the world offset in doubles.
export function createVoxelTerrain(
  content: Group, world: WorldInfo, palette: readonly Color[], chunkSurface: Material, pieceSurface: Material,
  occlusion: { keep(key: Point, faces: Int16Array | undefined): void },
) {
  const chunks = new Map<string, Mesh>();
  const pieces = new Map<number, Mesh>();
  const moved = new Vector3();
  let stale = true;

  function mesh(faces: Int16Array, surface: Material) {
    const geometry = voxelGeometry(faces, world, palette);
    geometry.setAttribute("meanNormal", new BufferAttribute(meanNormals(faces), 3, true));
    geometry.setAttribute("loose", new BufferAttribute(looseness(faces, world), 1, true));
    const part = new Mesh(geometry, surface);
    part.castShadow = true;
    part.receiveShadow = true;
    content.add(part);
    stale = true;
    return part;
  }

  function discard(part: Mesh | undefined) {
    if (!part) return;
    content.remove(part);
    part.geometry.dispose();
    stale = true;
  }

  function apply(frame: Frame) {
    for (const key of frame.unloaded) {
      const id = key.join(",");
      discard(chunks.get(id));
      chunks.delete(id);
      occlusion.keep(key, undefined);
    }
    for (const { key, origin, faces } of frame.chunks) {
      const id = key.join(",");
      discard(chunks.get(id));
      chunks.delete(id);
      occlusion.keep(key, faces);
      if (!faces.length) continue;
      const part = mesh(faces, chunkSurface);
      part.position.set(...origin);
      part.matrixAutoUpdate = false;
      part.updateMatrix();
      chunks.set(id, part);
    }
    for (const { id, faces } of frame.spawned) pieces.set(id, mesh(faces, pieceSurface));
    for (const id of frame.retired) {
      discard(pieces.get(id));
      pieces.delete(id);
    }
    for (const { id, position, rotation } of frame.debris) {
      const part = pieces.get(id);
      if (!part) continue;
      if (part.position.distanceToSquared(moved.set(...position)) > 0.000001 ||
          1 - Math.abs(part.quaternion.x * rotation[0] + part.quaternion.y * rotation[1]
            + part.quaternion.z * rotation[2] + part.quaternion.w * rotation[3]) > 0.000001) stale = true;
      part.position.set(...position);
      part.quaternion.set(...rotation);
    }
  }

  return {
    apply,
    // True once after any change that invalidates the shadow map.
    takeStale() {
      const was = stale;
      stale = false;
      return was;
    },
    dispose() {
      for (const part of chunks.values()) discard(part);
      for (const part of pieces.values()) discard(part);
      chunks.clear();
      pieces.clear();
    },
  };
}
