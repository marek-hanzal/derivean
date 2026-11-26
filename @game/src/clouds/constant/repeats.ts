// Metres after which each noise repeats. Broad shapes (storm cells, sheets) share one field through all bands so
// a tower stays in one piece; small shapes have one per band, low to high. The small repeats are whole multiples
// of the detail repeat, which lets detail reuse their offsets.
export const BroadRepeat = 48000;
export const SmallRepeats = [6000, 4000, 2000] as const;
export const DetailRepeat = 1000;
// How many times longer than wide a fully streaked cloud's structure is.
export const StreakStretch = 8;
