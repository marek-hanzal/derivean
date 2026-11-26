// Rows of the level table a level owns, and how many levels it has room for. Level 0 is the voxels themselves;
// the far levels are numbered as the engine numbers them (cube edge = voxelSize · 2^level).
export const LevelRows = 6;
export const LevelSlots = 16;

// The level of faces that belong to no level: detached pieces, drawn wherever they are.
export const FreeLevel = 15;

// Share of a cube by which a level is drawn beyond its box, over the coarser level that starts at the box: the
// two meet without a crack between them.
export const Overlap = 1 / 16;
