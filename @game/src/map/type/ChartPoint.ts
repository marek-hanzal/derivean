// One pixel of a chart, in the units of GameBridge's Chart.
export interface ChartPoint {
  ground: number;
  // NaN over dry ground.
  water: number;
  discharge: number;
  ice: number;
  warmth: number;
  cover: number;
  bedrock: number;
  waters: number;
  level: number;
}
