// Voxels per repeat of the per-voxel grain. Shaders read render-space positions, which differ from world positions
// by the render origin. The origin moves in whole repeats, so the grain stays fixed to the world at any distance
// from the world origin without world-sized numbers reaching float32.
export const GrainCells = 256;

// Grain repeats between two places of the render origin.
export const OriginRepeats = 16;

// Metres after which the ground's tonal variation repeats along the world's axes; a whole number of render
// origin steps.
export const GroundRepeat = 32768;
