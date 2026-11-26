import { type Group, Mesh } from "three";
import type { Frame } from "../../../contract/GameBridge";
import type { CloudShade } from "../../clouds/type/CloudShade";
import type { Air } from "../../sky/type/Air";
import type { SkyLight } from "../../sky/type/SkyLight";
import { disposeNaturalWaterGeometry, naturalWaterGeometry } from "../fn/naturalWaterGeometry";
import { naturalWaterSurface } from "../fn/naturalWaterSurface";

// Water the world generator placed (seas, lakes, rivers), drawn from the far patches' water heights. The hand-off
// to voxels concerns the ground alone, so these surfaces span the voxel box too.
export function createNaturalWater(content: Group, light: SkyLight, air: Air, shade: CloudShade, plain: { value: number }) {
  const material = naturalWaterSurface(light, air, shade, plain);
  const surfaces = new Map<number, Mesh>();
  const open = new Map<number, number>();
  let openCells = 0;

  function remove(id: number) {
    openCells -= open.get(id) ?? 0;
    open.delete(id);
    const surface = surfaces.get(id);
    if (!surface) return;
    content.remove(surface);
    disposeNaturalWaterGeometry(surface.geometry);
    surfaces.delete(id);
  }

  return {
    apply(frame: Frame) {
      for (const id of frame.far.removed) remove(id);
      for (const patch of frame.far.added) {
        remove(patch.id);
        const { geometry, open: left } = naturalWaterGeometry(patch);
        if (left) open.set(patch.id, left);
        openCells += left;
        if (!geometry) continue;
        const surface = new Mesh(geometry, material);
        surface.position.set(...patch.origin);
        surface.matrixAutoUpdate = false;
        surface.updateMatrix();
        surface.renderOrder = -1;
        content.add(surface);
        surfaces.set(patch.id, surface);
      }
    },
    // Cells of the patches held whose corners carry water levels too far apart to be one surface, and so are not
    // drawn.
    get openCells() { return openCells; },
    dispose() {
      for (const id of [...surfaces.keys(), ...open.keys()]) remove(id);
      material.dispose();
    },
  };
}
