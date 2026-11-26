import type { CloudField } from "../../../contract/GameBridge";
import { GenusNames } from "../constant/forms";

export interface WeatherSought {
  // A WMO genus by name, or "clear" for open sky.
  genus?: string;
  // Band to look in, lowest first; any when omitted.
  band?: number;
  // Least share of the sky the layer must cover.
  cover?: number;
  // Where the most rain or snow reaches the ground instead.
  fall?: "rain" | "snow";
}

// The weather cell that best shows what is sought in a snapshot, as the world position of its middle. It prefers
// cells whose neighbours show the same, so the view from there is of one system and not of its edge. This only
// chooses where to look; it changes nothing in the weather.
export function findWeather(field: CloudField, sought: WeatherSought) {
  const { cells, cell, origin } = field;
  const clear = (i: number) => field.bands.reduce((open, band) => open * (1 - Math.min(1, band.cover[i])), 1);
  const score = (i: number) => {
    if (sought.fall) return field[sought.fall][i];
    if (sought.genus === "clear") return clear(i) > 0.97 ? clear(i) : 0;
    const code = GenusNames.indexOf(sought.genus as typeof GenusNames[number]);
    let best = 0;
    field.bands.forEach((band, index) => {
      if (sought.band !== undefined && sought.band !== index) return;
      if (band.genus[i] === code && band.cover[i] >= (sought.cover ?? 0.05)) best = Math.max(best, band.cover[i]);
    });
    return best;
  };
  let found: { x: number; z: number; score: number } | undefined;
  for (let z = 0; z < cells; z++) {
    for (let x = 0; x < cells; x++) {
      const own = score(z * cells + x);
      if (own <= 0) continue;
      let company = 1;
      for (const [dx, dz] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        if (score(((z + dz + cells) % cells) * cells + (x + dx + cells) % cells) > 0) company++;
      }
      if (!found || own * company > found.score) found = { x: origin[0] + (x + 0.5) * cell, z: origin[1] + (z + 0.5) * cell, score: own * company };
    }
  }
  return found ? [found.x, found.z] as [number, number] : undefined;
}
