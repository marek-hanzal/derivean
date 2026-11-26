import { GrainCells, OriginRepeats } from "../constant/grain";

// Metres between two places of the render origin: a whole number of grain repeats.
export const renderOriginStep = (voxelSize: number) => GrainCells * voxelSize * OriginRepeats;
