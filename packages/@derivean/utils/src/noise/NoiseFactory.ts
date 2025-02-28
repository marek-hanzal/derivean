import type { Noise } from "./Noise";

export type NoiseFactory = (seed: string) => Noise;
