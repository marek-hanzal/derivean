import { levelView } from "../../sky/fn/skyLight";
import type { HazeAir } from "../../sky/type/HazeAir";

// The weather's haze over the eye as the shot draws it, and how far a dark target stays seen along a level line
// through that air at the sea and 3 km up, in km.
export function hazeRecord(air: Readonly<HazeAir> | null) {
  if (!air) return null;
  const km = (metres: number) => Number((metres / 1000).toFixed(1));
  return {
    depth: Number(air.haze.toFixed(4)), top: Math.round(air.top), thickness: air.thickness, veil: Number(air.veil.toFixed(4)),
    viewAtSeaKm: km(levelView(air, 0)), viewAt3kmKm: km(levelView(air, 3000)),
  };
}
