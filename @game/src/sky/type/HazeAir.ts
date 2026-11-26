// The weather's haze over a place (CloudField): optical depth at 550 nm of the low band's haze, spread evenly
// through `thickness` metres below `top` metres above the sea and on down into the valleys; and `veil`, the
// optical depth of ash and sulphate above it. `spectrum` is the haze's optical depth by colour (red, green, blue)
// against its depth at 550 nm, and `albedo` the share of what it takes from each colour that it scatters rather
// than absorbs. The veil is grey and scatters all it takes.
export interface HazeAir {
  haze: number;
  top: number;
  thickness: number;
  veil: number;
  spectrum: [number, number, number];
  albedo: [number, number, number];
}
