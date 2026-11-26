import type { WorldInfo } from "../../../contract/GameBridge";
import { formatAge } from "../../units/fn/age";

export function worldName(world: WorldInfo) {
  const name = `SEED ${world.seed} · ${formatAge(world.age).toUpperCase()}`;
  return world.origin ? `${world.origin} · ${name}` : name;
}
