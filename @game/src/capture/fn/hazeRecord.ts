import { levelView } from "../../sky/fn/skyLight";
import type { SeenAir } from "../../sky/type/SeenAir";

// The weather's haze over the eye as the shot draws it, and how far a dark target stays seen along a level line
// through that air at the ground of the haze's cell (the floor of its band), at the sea and 3 km up, in km.
export function hazeRecord(seen: SeenAir | null) {
  if (!seen) return null;
  const { haze: air, optics } = seen;
  const km = (metres: number) => Number((metres / 1000).toFixed(1));
  return {
    depth: Number(air.haze.toFixed(4)), top: Math.round(air.top), thickness: air.thickness, veil: Number(air.veil.toFixed(4)),
    spectrum: air.spectrum.map((value) => Number(value.toFixed(3))), albedo: air.albedo.map((value) => Number(value.toFixed(4))),
    viewAtGroundKm: km(levelView(air, air.top - air.thickness, optics)), viewAtSeaKm: km(levelView(air, 0, optics)),
    viewAt3kmKm: km(levelView(air, 3000, optics)),
  };
}
