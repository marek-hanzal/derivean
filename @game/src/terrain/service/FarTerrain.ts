import { type Group, Mesh, type MeshStandardMaterial, type Texture, Vector2 } from "three";
import type { FarPatch, Frame, WorldInfo } from "../../../contract/GameBridge";
import { TerrainLayer } from "../constant/shadow";
import { disposePatchGeometry, patchGeometry } from "../fn/patchGeometry";
import { patchSurface, type PatchLook } from "../fn/patchSurface";
import { shadeTexture } from "../fn/shadeTexture";

interface Placed { level: number; mesh: Mesh; material: MeshStandardMaterial; shade: Texture }
interface Level { band: { value: Vector2 }; count: number }

// Height-field terrain beyond the voxels: one mesh per engine patch, positioned at the patch origin, with a
// material of its own that carries the patch's shading raster.
export function createFarTerrain(content: Group, lod: WorldInfo["far"], look: PatchLook) {
  const patches = new Map<number, Placed>();
  const levels: (Level | undefined)[] = [];

  // A level leaves the screen at `far` metres from the eye and has taken its parent's shape by then.
  function level(patch: FarPatch) {
    const far = lod.splitCells * 2 * patch.size / patch.cells;
    return levels[patch.level] ??= { band: { value: new Vector2(far * lod.morphStart, far) }, count: 0 };
  }

  function remove(id: number) {
    const patch = patches.get(id);
    if (!patch) return;
    content.remove(patch.mesh);
    disposePatchGeometry(patch.mesh.geometry);
    patch.material.dispose();
    patch.shade.dispose();
    levels[patch.level]!.count--;
    patches.delete(id);
  }

  function add(patch: FarPatch) {
    const owner = level(patch);
    const shade = shadeTexture(patch);
    const texels = patch.cells * patch.shadeDetail;
    const material = patchSurface(look, owner.band, shade, texels, patch.size / texels);
    const mesh = new Mesh(patchGeometry(patch), material);
    mesh.position.set(...patch.origin);
    mesh.matrixAutoUpdate = false;
    mesh.updateMatrix();
    mesh.layers.enable(TerrainLayer);
    content.add(mesh);
    owner.count++;
    patches.set(patch.id, { level: patch.level, mesh, material, shade });
  }

  return {
    // Returns whether the terrain changed.
    apply(frame: Frame) {
      for (const id of frame.far.removed) remove(id);
      for (const patch of frame.far.added) {
        remove(patch.id);
        add(patch);
      }
      return frame.far.added.length + frame.far.removed.length > 0;
    },
    counts: () => Array.from(levels, (level) => level?.count ?? 0),
    dispose() {
      for (const id of [...patches.keys()]) remove(id);
      levels.length = 0;
    },
  };
}
