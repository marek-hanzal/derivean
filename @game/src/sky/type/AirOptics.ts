import type { WorldInfo } from "../../../contract/GameBridge";

// The simulation's clean air (WorldInfo.air): per metre at sea level by colour, red, green and blue, and the
// metres over which it thins by e.
export type AirOptics = Readonly<WorldInfo["air"]>;
