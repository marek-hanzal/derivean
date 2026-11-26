import type { GameBridge } from "../../../contract/GameBridge";

// A ring round the eye, wide enough that its pixels can just show a level of detail, and that level as charts
// number it.
const Rings = [
  { name: "site", span: 1024, level: 3 },
  { name: "local", span: 16_384, level: 2 },
  { name: "region", span: 262_144, level: 1 },
] as const;
const Pixels = 256;

// How finished the simulation's data round a point is, for judging a capture: the share of the ground within
// half a kilometre held at site detail, within 8 km at local detail or finer, and within 131 km at region detail
// or finer. A capture taken below 1 shows coarser stand-ins somewhere in its view.
export async function dataLevels(bridge: Pick<GameBridge, "chart">, x: number, z: number) {
  const shares: Record<string, number> = {};
  for (const { name, span, level } of Rings) {
    const chart = await bridge.chart({ x, z, span, size: Pixels });
    let held = 0;
    for (const read of chart?.level ?? []) if (read >= level) held++;
    shares[name] = Number((held / Pixels ** 2).toFixed(3));
  }
  return shares;
}
