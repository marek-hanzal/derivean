import { CleanAir, CleanAirHeight, HazeAir, HazeAirHeight, HighHaze, HighHazeHeight } from "../constant/air";

// Share of the light the air scatters out of the sun's beam that reaches the ground, and of what the haze scatters
// that goes forward (Bird and Hulstrom's clear-sky model, 1981; clean air scatters half of it forward).
const Reaching = 0.79;
const HazeForward = 0.84;

export interface ClearSky {
  // Of the sun's light outside the air, what reaches the ground straight, by colour.
  direct: [number, number, number];
  // Of the same, what the clear sky sends onto level ground, by colour.
  diffuse: [number, number, number];
}

// Air masses the sun's light crosses at a height over the horizon (`up`, the sine of it): Kasten and Young (1989).
export function airMass(up: number) {
  const degrees = Math.max(up, 0) * 180 / Math.PI;
  return 1 / (Math.max(up, 0) + 0.50572 * (degrees + 6.07995) ** -1.6364);
}

// The clear sky's light over ground `altitude` metres above the sea, with the sun `up` (sine of its height): the
// air drawn by the aerial perspective, its clean air and its haze thinning with height, scatters the sun's beam,
// and Bird and Hulstrom's model gives what is left of the beam and what of the scattered light falls on level
// ground. The same air, so the low sun's light and its shadows follow from what the view shows of the air.
export function clearSky(up: number, altitude: number): ClearSky {
  const out: ClearSky = { direct: [0, 0, 0], diffuse: [0, 0, 0] };
  if (up <= 0) return out;
  const mass = airMass(up);
  const above = Math.max(altitude, 0);
  const haze = HazeAir * HazeAirHeight * Math.exp(-above / HazeAirHeight) + HighHaze * Math.exp(-above / HighHazeHeight);
  const hazeThrough = Math.exp(-haze * mass);
  // Light scattered more than once reaches the ground from a low sun through this much more air.
  const spread = 1 - mass + mass ** 1.02;
  for (let colour = 0; colour < 3; colour++) {
    const clean = CleanAir[colour] * CleanAirHeight * Math.exp(-above / CleanAirHeight);
    const cleanThrough = Math.exp(-clean * mass);
    out.direct[colour] = cleanThrough * hazeThrough;
    out.diffuse[colour] = Reaching * up * (0.5 * (1 - cleanThrough) + HazeForward * (1 - hazeThrough)) / spread;
  }
  return out;
}
