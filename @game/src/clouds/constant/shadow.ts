// Side in metres and resolution of the map of cloud shadows around the eye, and how often it is redrawn.
export const ShadowSpan = 96000;
export const ShadowSize = 1024;
export const ShadowMs = 200;

// How much direct sunlight comes out below cloud (fn/sunThrough): at least this share, and of the rest this share
// scattered forward, which thins this many times slower than the straight beam. The map's shader spells the same out
// in WGSL from these numbers.
export const LeastThrough = 0.12;
export const ForwardShare = 0.3;
export const ForwardThinning = 0.25;
