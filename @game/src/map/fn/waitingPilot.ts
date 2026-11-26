import type { Point } from "../../../contract/GameBridge";
import type { Pilot } from "../../control/type/Pilot";

// Wraps whoever steers the player. While `holding` is true the world keeps running under a pilot that only
// waits, or travels once to the point `travel` hands out.
export function waitingPilot(pilot: Pilot, holding: () => boolean, travel: () => Point | null): Pilot {
  return {
    get yaw() { return pilot.yaw; },
    get pitch() { return pilot.pitch; },
    get radius() { return pilot.radius; },
    get shape() { return pilot.shape; },
    active: () => holding() || pilot.active(),
    command(elapsed, time, frame) {
      const command = pilot.command(elapsed, time, frame);
      if (!holding()) return command;
      return { ...command, active: true, x: 0, z: 0, jump: false, sprint: false, dig: false, place: false, fly: false, boost: false, ascend: 0, teleport: travel() };
    },
  };
}
