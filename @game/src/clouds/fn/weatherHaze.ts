import type { CloudField } from "../../../contract/GameBridge";
import type { HazeAir } from "../../sky/type/HazeAir";

// The four cells round the position last read and their weights: (0, 0), (1, 0), (0, 1), (1, 1).
const corners = [0, 0, 0, 0];
const weights = [0, 0, 0, 0];

// A value linear between those four cells.
function weighed(values: Float32Array) {
  let sum = 0;
  for (let corner = 0; corner < 4; corner++) sum += values[corners[corner]] * weights[corner];
  return sum;
}

// A ratio of hazeColour's `entry` over those four cells, each weighed by its own denominator: the depth at 550 nm
// for the spectrum (`colour` -1 or 1), the depth at that colour for the albedo. hazeColour holds `stride` entries a
// cell (the contract's five).
function ratio(hazeDepth: Float32Array, hazeColour: Float32Array, stride: number, entry: number, colour: number) {
  let sum = 0, total = 0;
  for (let corner = 0; corner < 4; corner++) {
    const spot = corners[corner];
    const over = colour < 0 || colour === 1 ? hazeDepth[spot] : hazeDepth[spot] * hazeColour[spot * stride + (colour === 0 ? 0 : 1)];
    const share = weights[corner] * Math.max(over, 1e-6);
    sum += hazeColour[spot * stride + entry] * share;
    total += share;
  }
  return sum / total;
}

// The haze of the weather over a world position, linear between cell centres, into `air`: optical depth of the low
// band's haze, the height of its top above the sea and its thickness, and the veil above it. The haze's colour is
// weighed by each cell's haze.
export function weatherHaze(field: CloudField, x: number, z: number, air: HazeAir) {
  const { cells, cell, origin, hazeDepth, hazeTop, hazeColour, aerosolDepth } = field;
  const fx = (x - origin[0]) / cell - 0.5, fz = (z - origin[1]) / cell - 0.5;
  const x0 = Math.floor(fx), z0 = Math.floor(fz), tx = fx - x0, tz = fz - z0;
  for (let corner = 0; corner < 4; corner++) {
    const [dx, dz] = [corner & 1, corner >> 1];
    corners[corner] = (((z0 + dz) % cells + cells) % cells) * cells + ((x0 + dx) % cells + cells) % cells;
    weights[corner] = (dx ? tx : 1 - tx) * (dz ? tz : 1 - tz);
  }
  const stride = hazeColour.length / hazeDepth.length;
  air.haze = weighed(hazeDepth);
  air.top = weighed(hazeTop);
  air.thickness = field.hazeThickness;
  air.veil = weighed(aerosolDepth);
  air.spectrum[0] = ratio(hazeDepth, hazeColour, stride, 0, -1);
  air.spectrum[1] = 1;
  air.spectrum[2] = ratio(hazeDepth, hazeColour, stride, 1, -1);
  air.albedo[0] = ratio(hazeDepth, hazeColour, stride, 2, 0);
  air.albedo[1] = ratio(hazeDepth, hazeColour, stride, 3, 1);
  air.albedo[2] = ratio(hazeDepth, hazeColour, stride, 4, 2);
  return air;
}
