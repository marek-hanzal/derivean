import { sampler } from "three/tsl";
import { CloudSkyFields } from "../constant/density";
import type { CloudNodes } from "../type/CloudNodes";

// The WGSL parameters every function that builds a CloudSky takes: its fields (constant/density), and the textures
// passed on with it.
export const CloudTextures = "bands, bandSampler, shapes, details, repeat, levels";
export const CloudParams = [
  ...CloudSkyFields.map(([name, type]) => `${name}: ${type}`),
  "bands: texture_2d_array<f32>, bandSampler: sampler, shapes: texture_3d<f32>, details: texture_3d<f32>, repeat: sampler",
  "levels: texture_2d<f32>",
].join(", ");
export const CloudSkyOf = `CloudSky(${CloudSkyFields.map(([name]) => name).join(", ")})`;

// The nodes for those parameters, by name.
export function cloudArgs(clouds: CloudNodes) {
  return {
    grid: clouds.grid, steer: clouds.steer, datum: clouds.datum,
    range0: clouds.ranges[0], range1: clouds.ranges[1], range2: clouds.ranges[2],
    drift0: clouds.drifts[0], drift1: clouds.drifts[1], drift2: clouds.drifts[2],
    wind01: clouds.wind01, wind2: clouds.wind2,
    bands: clouds.bands, bandSampler: sampler(clouds.bands), shapes: clouds.shapes, details: clouds.details,
    repeat: sampler(clouds.shapes), levels: clouds.levels,
  };
}
