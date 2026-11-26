import type { WorldInfo } from "../../../contract/GameBridge";
import { formatAge } from "../../units/fn/age";

export function worldName(world: WorldInfo) {
  return `SEED ${world.seed} · ${formatAge(world.age).toUpperCase()}`;
}
