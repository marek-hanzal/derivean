// Defaults of a capture plan, in seconds.
export const SettleSeconds = 2;
export const TimeoutSeconds = 120;
// A shot's timeout counts from when the world is playable where it stands, but from this long after it got there
// at the latest, so a spot that never streams still ends in a shot.
export const PlayableSeconds = 300;
// Pending work that delivers nothing for this long is taken as stuck rather than waited out.
export const StallSeconds = 8;
