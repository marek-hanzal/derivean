import { Vector3 } from "three";
import { GrainCells, GroundRepeat, OriginRepeats } from "../constant/grain";
import type { SurfaceTone } from "../type/SurfaceTone";

// Metres between two places of the render origin: a whole number of grain repeats.
export const renderOriginStep = (voxelSize: number) => GrainCells * voxelSize * OriginRepeats;

export function surfaceTone(voxelSize: number): SurfaceTone {
  return {
    plain: { value: 0 },
    grainCells: { value: GrainCells },
    voxelSize: { value: voxelSize },
    groundAnchor: { value: new Vector3() },
  };
}

// Places the ground's variation for a render origin in world metres.
export function anchorTone(tone: SurfaceTone, origin: Vector3) {
  const within = (value: number) => value - Math.floor(value / GroundRepeat) * GroundRepeat;
  tone.groundAnchor.value.set(within(origin.x), within(origin.y), within(origin.z));
}
