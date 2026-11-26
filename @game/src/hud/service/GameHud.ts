import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import type { Pilot } from "../../control/type/Pilot";
import { select } from "../../dom/fn/select";
import { worldName } from "../fn/worldName";

export function createGameHud(hud: HTMLElement) {
  const crosshair = select(hud, ".crosshair");
  const worldTag = select(hud, "#world-tag");
  const materialName = select(hud, "#material");
  const stock = select(hud, "#stock");
  const brushSize = select(hud, "#brush-size");
  const sphere = select(hud, "#sphere");
  const cube = select(hud, "#cube");

  return {
    world(world: WorldInfo) {
      worldTag.textContent = worldName(world);
      if (world.replay) document.title = `DeRivean · REPLAY ${world.replay}`;
    },
    paint(world: WorldInfo, frame: Frame, pilot: Pilot) {
      const material = frame.target ? world.materials[frame.target.material] : undefined;
      const strength = material?.matter === "granular" ? `${material.angleOfRepose}° repose` : `${material?.support} kg/face`;
      const diameter = (2 * pilot.radius - Number(pilot.shape === "cube")) * world.voxelSize;
      hud.dataset.flying = String(frame.flying);
      crosshair.dataset.target = String(frame.target !== null);
      stock.textContent = String(frame.buildStock);
      materialName.textContent = material ? `${material.name} · ${material.density} kg/m³ · ${strength}` : "—";
      brushSize.textContent = `${diameter.toFixed(2)} m`;
      sphere.dataset.selected = String(pilot.shape === "sphere");
      cube.dataset.selected = String(pilot.shape === "cube");
    },
  };
}
