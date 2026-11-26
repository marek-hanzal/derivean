// The weather's haze over a place (CloudField): optical depth at 550 nm of the low band's haze, spread evenly
// through `thickness` metres below `top` metres above the sea and on down into the valleys; and `veil`, the
// optical depth of ash and sulphate above it.
export interface HazeAir {
  haze: number;
  top: number;
  thickness: number;
  veil: number;
}
