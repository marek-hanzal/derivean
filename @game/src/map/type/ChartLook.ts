// How a chart is drawn so that it reads the same over mountains and over a plain, at any zoom.
export interface ChartLook {
  // m³/yr below which a stream is left out; infinite where the chart has none.
  streamFloor: number;
  // Vertical exaggeration of the relief shading.
  relief: number;
}

// What one pane says about how its chart should be drawn; panes are measured one at a time as they arrive.
export interface PaneMeasure {
  // Dry pixels by the power of two of their flow in m³/yr, and how many were counted.
  flows: Uint32Array;
  dry: number;
  // The smallest flow the simulation calls a stream in the pane.
  least: number;
  // Sum of squared slopes, and of how many pixels.
  squares: number;
  slopes: number;
}
