import { LandReach, NoHeight, SlopeMargin, SlopeWords } from "../constant/land";

// The slope of the land at the columns [x0, x1) × [z0, z1) of a square of `side` columns, as landNormal reads it.
// `around` holds the heights of the square's columns and of SlopeMargin columns round it, row by row along x,
// NoHeight where none is held; a column not held reads as the one measured on, so the slope is measured on what is
// there. `into` takes SlopeWords numbers a column of the square, row by row; a column with no height gets NoHeight
// and no slope. The rise across a reach is the mean of three rows of columns: the column's own and one to either
// side. The sums and quotients are rounded as the GPU's 32-bit floats round them.
export function landSlopes(around: Float32Array, side: number, into: Float32Array, x0: number, x1: number, z0: number, z1: number) {
  const width = side + 2 * SlopeMargin;
  const reach = (centre: number, own: number, step: number, cross: number, distance: number) => {
    let rise = 0;
    for (let beside = -1; beside <= 1; beside++) {
      const ahead = around[centre + distance * step + beside * cross];
      const behind = around[centre - distance * step + beside * cross];
      rise = Math.fround(rise + Math.fround((ahead > NoHeight / 2 ? ahead : own) - (behind > NoHeight / 2 ? behind : own)));
    }
    return Math.fround(rise / (3 * 2 * distance));
  };
  for (let z = z0; z < z1; z++) {
    for (let x = x0; x < x1; x++) {
      const centre = (z + SlopeMargin) * width + x + SlopeMargin;
      const own = around[centre];
      const out = (z * side + x) * SlopeWords;
      if (!(own > NoHeight / 2)) {
        into.fill(0, out, out + SlopeWords);
        into[out] = NoHeight;
        continue;
      }
      const nearX = reach(centre, own, 1, width, LandReach[0]);
      const nearZ = reach(centre, own, width, 1, LandReach[0]);
      into[out] = own;
      into[out + 1] = Math.hypot(nearX, nearZ);
      into[out + 2] = Math.fround(nearX + reach(centre, own, 1, width, LandReach[1])) * 0.5;
      into[out + 3] = Math.fround(nearZ + reach(centre, own, width, 1, LandReach[1])) * 0.5;
    }
  }
}
