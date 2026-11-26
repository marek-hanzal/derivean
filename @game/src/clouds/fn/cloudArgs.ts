import { sampler } from "three/tsl";
import type { CloudNodes } from "../type/CloudNodes";

// The WGSL parameters every function that builds a CloudSky takes, by name:
//   grid: vec4f, steer: vec2f, datum: f32, range0..2: vec4f, drift0..2: vec4f, wind01: vec4f, wind2: vec2f,
//   bands: texture_2d_array<f32>, bandSampler: sampler, shapes: texture_3d<f32>, details: texture_3d<f32>,
//   repeat: sampler, levels: texture_2d<f32>
export const CloudParams = `grid: vec4f, steer: vec2f, datum: f32, range0: vec4f, range1: vec4f, range2: vec4f, drift0: vec4f, drift1: vec4f,
  drift2: vec4f, wind01: vec4f, wind2: vec2f, bands: texture_2d_array<f32>, bandSampler: sampler, shapes: texture_3d<f32>,
  details: texture_3d<f32>, repeat: sampler, levels: texture_2d<f32>`;

// The struct those parameters make, and the textures passed on with it.
export const CloudSkyOf = "CloudSky(grid, steer, datum, range0, range1, range2, drift0, drift1, drift2, wind01, wind2)";
export const CloudTextures = "bands, bandSampler, shapes, details, repeat, levels";

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
