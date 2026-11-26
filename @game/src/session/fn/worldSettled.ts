import type { Frame } from "../../../contract/GameBridge";

// Nothing is left to stream for where the player is: every level of detail is in and no chunk or patch is due.
export function worldSettled(frame: Frame) {
  return frame.streamingReady && frame.ready.site && frame.ready.local && frame.ready.region
    && frame.pending.chunks === 0 && frame.pending.patches === 0;
}
