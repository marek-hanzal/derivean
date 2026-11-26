import type { Frame } from "../../../contract/GameBridge";
import { worldSettled } from "../../session/fn/worldSettled";

// What the engine is still doing, in the launcher's words.
export function streamingProgress(frame: Frame) {
  const { ready, pending } = frame;
  const waiting = [
    ...(pending.chunks ? [`${pending.chunks} voxel chunks`] : []),
    ...(pending.patches ? [`${pending.patches} terrain patches`] : []),
  ].join(" and ");
  if (!frame.streamingReady) {
    const stage = !ready.site ? "Building the ground under the spawn" : !ready.local ? "Streaming nearby terrain" : "Streaming the region";
    return `${stage}${waiting ? ` · ${waiting} to go` : ""}`;
  }
  if (worldSettled(frame)) return "World ready";
  return `Ready to enter · terrain still refining${waiting ? ` (${waiting} to go)` : ""}`;
}
