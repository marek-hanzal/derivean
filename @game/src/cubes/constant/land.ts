// Squares of ground (chunks, 32 columns) along each side of the ring of ground heights a far level keeps
// (LevelHeights). It holds a level's box and band at the reach the engine uses by default (2 · 128 + 2 · 4
// columns); what a level holds beyond that shares places with the ring's other side, and the nearer square keeps
// them.
export const HeightBlocks = 16;

// A column with no height held.
export const NoHeight = -1e30;

// Columns to either side over which the slope of the land is measured, a near and a far reach averaged: a coarse
// step stands for a slope only as wide as the steps around it, so the far reach has to span a few of them on
// gentle ground, and the near one keeps a ridge or a valley from being smeared across.
export const LandReach = [3, 8] as const;

// Cube edges, metres, over which faces start to be lit as the land they stand for, ramped by the number of
// doublings, and the most of their light that is the land's: a cube a few voxels across is still the voxels'
// own, a coarse one stands for many of them.
export const LandEdges = [2, 16] as const;
export const LandShare = 0.75;

// Slopes, rise over run, between which the land stops being lit as a slope and its cubes keep their own light:
// the engine takes ground steeper than 45° for rock that stands as a face (CubeChunk.quads), and so does this.
export const Steep = [0.8, 1.2] as const;

// Slopes, rise over run (40° to 45°), above which snow stops lying on the land. A riser on gentler ground is the
// stair the lattice cuts into a slope the snow covers, so it is drawn with the snow its top carries; steeper, it
// is a face of rock and stays bare.
export const SnowHolds = [0.84, 1] as const;

// Cubes, of the level the slope is measured on, that a riser's cube may lie below the top of its column and still
// take the snow: a step at the surface is one cube high, or two where the slope steepens.
export const SnowDepth = 1;
