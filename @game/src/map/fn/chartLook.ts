import type { Chart } from "../../../contract/GameBridge";
import { ChartWater } from "../constant/chart";
import { StreamShare, TypicalShade } from "../constant/map";
import type { ChartLook, PaneMeasure } from "../type/ChartLook";

const Stride = 3;
const Buckets = 64;

export function measurePane({ size, pitch, ground, discharge, waters }: Chart): PaneMeasure {
  const measure: PaneMeasure = { flows: new Uint32Array(Buckets), dry: 0, least: Infinity, squares: 0, slopes: 0 };
  for (let pixel = 0; pixel < waters.length; pixel++) {
    if (waters[pixel] === ChartWater.Stream && discharge[pixel] < measure.least) measure.least = discharge[pixel];
  }
  for (let row = 1; row < size - 1; row += Stride) {
    for (let pixel = row * size + 1; pixel < (row + 1) * size - 1; pixel += Stride) {
      const dx = (ground[pixel + 1] - ground[pixel - 1]) / (2 * pitch);
      const dz = (ground[pixel + size] - ground[pixel - size]) / (2 * pitch);
      measure.squares += dx * dx + dz * dz;
      measure.slopes++;
      if (waters[pixel] === ChartWater.Sea || waters[pixel] === ChartWater.Lake) continue;
      measure.dry++;
      measure.flows[Math.min(Buckets - 1, Math.max(0, Math.floor(Math.log2(Math.max(1, discharge[pixel])))))]++;
    }
  }
  return measure;
}

// The look of a chart from the measures of its panes. Streams: at least the smallest flow the simulation itself
// calls a stream in this chart, and no more than the largest StreamShare of dry pixels, so that a plain where
// every cell runs with water still shows its rivers and not a blue sheet. Relief: the exaggeration at which the
// root-mean-square slope shades like TypicalShade, since slopes fall as a pixel takes in more ground and differ
// by orders of magnitude between a planet's cells and a site's.
export function chartLook(measures: PaneMeasure[]): ChartLook {
  const flows = new Uint32Array(Buckets);
  let least = Infinity;
  let dry = 0;
  let squares = 0;
  let slopes = 0;
  for (const measure of measures) {
    measure.flows.forEach((count, bucket) => { flows[bucket] += count; });
    least = Math.min(least, measure.least);
    dry += measure.dry;
    squares += measure.squares;
    slopes += measure.slopes;
  }
  // From the greatest flows down, as many whole classes as fit the share; the greatest class always shows.
  let bucket = Buckets - 1;
  while (bucket > 0 && !flows[bucket]) bucket--;
  for (let above = flows[bucket]; bucket > 0 && above + flows[bucket - 1] <= StreamShare * dry; bucket--) above += flows[bucket - 1];
  const slope = Math.sqrt(squares / Math.max(1, slopes));
  return {
    streamFloor: Math.max(least, 2 ** bucket),
    relief: Math.max(1, Math.min(60, TypicalShade / Math.max(1e-6, slope))),
  };
}
