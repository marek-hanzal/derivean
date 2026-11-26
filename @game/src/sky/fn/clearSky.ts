import { CleanAir, CleanAirHeight, HazeAir, HazeAirHeight, HighHaze, HighHazeHeight } from "../constant/air";

// Share of the light the air scatters out of the sun's beam that reaches the ground, and of what the haze scatters
// that goes forward (Bird and Hulstrom's clear-sky model, 1981; clean air scatters half of it forward).
const Reaching = 0.79;
const HazeForward = 0.84;
// Bird and Hulstrom's sky light falls with the sine of the sun's height and would leave the ground black at
// sunset. At a sun 1° up it gives what Earth's clear sky gives about then (about 0.6 % of noon); below that the
// sky light falls tenfold for every 2.6° the sun sinks, as Earth's does in civil twilight.
const TwilightFrom = 1;
const TwilightFall = 2.6;

export interface ClearSky {
  // Of the sun's light outside the air, what reaches the ground straight, by colour.
  direct: [number, number, number];
  // Of the same, what the clear sky sends onto level ground, by colour.
  diffuse: [number, number, number];
}

// Air masses the sun's light crosses at `degrees` over the horizon: Kasten and Young (1989).
function airMass(degrees: number) {
  return 1 / (Math.sin(degrees * Math.PI / 180) + 0.50572 * (degrees + 6.07995) ** -1.6364);
}

// The clear sky's light over ground `altitude` metres above the sea, with the sun `up` (sine of its height): the
// air drawn by the aerial perspective, its clean air and its haze thinning with height, scatters the sun's beam,
// and Bird and Hulstrom's model gives what is left of the beam and what of the scattered light falls on level
// ground. The same air, so the low sun's light and its shadows follow from what the view shows of the air.
export function clearSky(up: number, altitude: number): ClearSky {
  const out: ClearSky = { direct: [0, 0, 0], diffuse: [0, 0, 0] };
  const height = Math.asin(Math.max(-1, Math.min(1, up))) * 180 / Math.PI;
  // The sun's height the model is evaluated at, and what the twilight leaves of that below it.
  const degrees = Math.max(height, TwilightFrom);
  const twilight = height < TwilightFrom ? 10 ** ((height - TwilightFrom) / TwilightFall) : 1;
  const mass = airMass(degrees);
  const above = Math.max(altitude, 0);
  const haze = HazeAir * HazeAirHeight * Math.exp(-above / HazeAirHeight) + HighHaze * Math.exp(-above / HighHazeHeight);
  const hazeThrough = Math.exp(-haze * mass);
  // Light scattered more than once reaches the ground from a low sun through this much more air.
  const spread = 1 - mass + mass ** 1.02;
  const sine = Math.sin(degrees * Math.PI / 180);
  // The beam's own path: none once the sun is down.
  const beam = height > 0 ? airMass(height) : Infinity;
  for (let colour = 0; colour < 3; colour++) {
    const clean = CleanAir[colour] * CleanAirHeight * Math.exp(-above / CleanAirHeight);
    const cleanThrough = Math.exp(-clean * mass);
    out.diffuse[colour] = twilight * Reaching * sine * (0.5 * (1 - cleanThrough) + HazeForward * (1 - hazeThrough)) / spread;
    out.direct[colour] = Math.exp(-(clean + haze) * beam);
  }
  return out;
}
