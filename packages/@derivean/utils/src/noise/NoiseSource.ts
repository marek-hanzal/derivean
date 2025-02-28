import type { Noise } from "./Noise";
import type { NoiseType } from "./NoiseType";

/**
 * Define individual noises to make up a biome.
 */
export type NoiseSource = ({ seed }: { seed: string }) => Record<NoiseType, Noise>;
