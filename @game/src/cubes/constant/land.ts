// Columns along each side of the ring of ground heights a far level keeps (LevelHeights): a power of two and a
// whole number of chunks, wider than a level's box and band (2 · reach + 2 · band cubes, plus the chunk the box
// edge cuts) so that no two held columns share a place.
export const HeightSide = 512;

// A column with no height held.
export const NoHeight = -1e30;

// Columns to either side over which the slope of the land is measured: a coarse step stands for a slope only
// as wide as the steps around it, so the reach has to span a few of them.
export const LandReach = 4;

// Cube edges, metres, over which faces start to be lit as the land they stand for, and the most of their light
// that is the land's: a cube a few voxels across is still the voxels' own, a coarse one stands for many of them.
export const LandEdges = [2, 16] as const;
export const LandShare = 0.75;

// Slopes, rise over run, between which the land stops being lit as a slope and its cubes keep their own light:
// the engine takes ground steeper than 45° for rock that stands as a face (CubeChunk.quads).
export const Steep = [0.8, 1.2] as const;
