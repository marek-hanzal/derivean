import type { Frame, WorldInfo } from "../../../contract/GameBridge";
import type { Pilot } from "../../control/type/Pilot";
import { select } from "../../dom/fn/select";
import { worldName } from "../fn/worldName";

// The HUD is painted every frame and changes seldom: a text or a data attribute is written only when it differs, so
// an unchanged HUD costs the page no style or layout work.
function text(element: HTMLElement, value: string) {
  if (element.textContent !== value) element.textContent = value;
}
function data(element: HTMLElement, key: string, value: string) {
  if (element.dataset[key] !== value) element.dataset[key] = value;
}

export function createGameHud(hud: HTMLElement) {
  const crosshair = select(hud, ".crosshair");
  const worldTag = select(hud, "#world-tag");
  const materialName = select(hud, "#material");
  const stock = select(hud, "#stock");
  const brushSize = select(hud, "#brush-size");
  const sphere = select(hud, "#sphere");
  const cube = select(hud, "#cube");

  return {
    world(world: WorldInfo) { worldTag.textContent = worldName(world); },
    paint(world: WorldInfo, frame: Frame, pilot: Pilot) {
      const material = frame.target ? world.materials[frame.target.material] : undefined;
      const strength = material?.matter === "granular" ? `${material.angleOfRepose}° repose` : `${material?.support} kg/face`;
      const diameter = (2 * pilot.radius - Number(pilot.shape === "cube")) * world.voxelSize;
      data(hud, "flying", String(frame.flying));
      data(crosshair, "target", String(frame.target !== null));
      text(stock, String(frame.buildStock));
      text(materialName, material ? `${material.name} · ${material.density} kg/m³ · ${strength}` : "—");
      text(brushSize, `${diameter.toFixed(2)} m`);
      data(sphere, "selected", String(pilot.shape === "sphere"));
      data(cube, "selected", String(pilot.shape === "cube"));
    },
  };
}
