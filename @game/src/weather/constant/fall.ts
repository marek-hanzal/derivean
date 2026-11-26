// How rain and snow are drawn. The rates come from the simulated weather; these only say what a drop looks like.
export interface Fall {
  // Most drops drawn at once, in the box around the eye.
  drops: number;
  // Rate in mm of water an hour at which all of them are out.
  heaviest: number;
  // Metres a second downwards.
  speed: number;
  // Seconds of travel a drop is smeared over, and its width in metres.
  trail: number;
  width: number;
  // Metres a flake wanders sideways.
  sway: number;
  opacity: number;
}

export const Rain: Fall = { drops: 9000, heaviest: 30, speed: 9, trail: 0.03, width: 0.004, sway: 0, opacity: 0.22 };
export const Snow: Fall = { drops: 7000, heaviest: 6, speed: 1.3, trail: 0.004, width: 0.012, sway: 0.25, opacity: 0.8 };

// Side in metres of the cube around the eye that drops are kept in.
export const FallBox = 32;
// Metres below the simulated ground at which the eye is taken to be under it, where nothing falls.
export const Shelter = 3;
