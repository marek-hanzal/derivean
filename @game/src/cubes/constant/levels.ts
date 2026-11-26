// Rows of the level table a level owns, and how many levels it has room for. Level 0 is the voxels themselves;
// the far levels are numbered as the engine numbers them (cube edge = voxelSize · 2^level).
export const LevelRows = 6;
export const LevelSlots = 16;

// The level of faces that belong to no level: detached pieces, drawn wherever they are.
export const FreeLevel = 15;

// Share of a cube of the coarser level by which a level is drawn beyond its square, over the coarser level that
// starts there: the two meet without a crack between them. The coarser level's caps stand within it (faceNodes).
export const Overlap = 1 / 16;
