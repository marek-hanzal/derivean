import type { BrushShape, Command, Frame } from "../../../contract/GameBridge";

// A command without the clock, which the session adds from the world clock.
export type Steering = Omit<Command, "time">;

// Whoever steers the player: the keyboard and mouse, or a scripted capture.
export interface Pilot {
  readonly yaw: number;
  readonly pitch: number;
  readonly radius: number;
  readonly shape: BrushShape;
  // True while the simulation should run and the view should follow the player.
  active(): boolean;
  // `frame` is the engine's answer to the previous command, if any.
  command(elapsed: number, time: number, frame: Frame | undefined): Steering;
}
