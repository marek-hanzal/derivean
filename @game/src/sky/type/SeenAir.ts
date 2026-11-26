import type { AirOptics } from "./AirOptics";
import type { HazeAir } from "./HazeAir";

// The air the light and the aerial perspective are drawn with: the weather's haze over the eye and the clean air.
export interface SeenAir {
  haze: Readonly<HazeAir>;
  optics: AirOptics;
}
