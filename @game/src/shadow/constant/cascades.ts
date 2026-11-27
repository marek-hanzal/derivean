// The sun's shadows are kept in cascades (Dimitrov 2007): depth maps seen from the sun (Williams 1978; both in
// docs/credits/rendering.md), each centred on the ground under the eye and each holding the levels of cubes whose
// cubes are about as wide as its texels. A level's box reaches a fixed number of its cubes from the eye, so four
// levels to a cascade and a map twice as wide as the coarsest of them make a texel as wide as a cube of the
// finest: a step one cube high then still casts a shadow a texel wide.
export const LevelsPerCascade = 4;
export const Cascades = 4;

// Texels along the edge of each cascade's map. The last holds the two coarsest levels over the whole view.
export const MapSizes = [4096, 4096, 4096, 2048] as const;

// Half the width of a cascade's map over half the width of its coarsest level's box: room for the box seen
// corner-on from the sun and for ground that lies higher or lower than the ground under the eye. The last
// cascade spans the whole view, where heights no longer count.
export const MapRoom = [2, 2, 2, 1.5] as const;

// Depth of a cascade's map to either side of its centre, over half the width of its coarsest level's box.
export const MapDepth = 4;

// Share of the half width of its coarsest level's box the eye may move before a cascade is drawn again, and the
// radians the sun may.
export const MoveShare = 1 / 8;
export const SunTurn = 0.002;

// Frames a cascade waits at least between two draws of itself. Only one cascade is drawn in a frame, except the
// ones that have never been drawn.
export const DrawEvery = [1, 4, 16, 64] as const;

// Share of a map's half width over which its shadows fade out towards its border.
export const EdgeFade = 0.1;

// Texels by which a receiver is taken to lie nearer the sun than it does, against ties with the casters.
export const TexelBias = 0.05;

// The cascade a level of cubes casts into. Detached pieces, which belong to no level, cast into the finest.
export const cascadeOf = (level: number) => Math.min(Cascades - 1, Math.floor(level / LevelsPerCascade));
