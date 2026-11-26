// Where the map looks: the world position under the middle of the viewport, and metres per CSS pixel. World
// metres stay in JavaScript numbers; only differences from a chart's corner ever reach 32-bit floats.
export interface MapView {
  x: number;
  z: number;
  scale: number;
}

// A world-aligned square in metres, the shape charts are asked for in.
export interface Square {
  x: number;
  z: number;
  span: number;
}

// Viewport width and height in CSS pixels.
export type Extent = [number, number];
