/** @format */

import type { XZ } from "../type/XZ";
import type { NoiseType } from "./NoiseType";

export namespace NoiseSource {
	export type Noise = Record<NoiseType, number>;

	export interface Props {
		seed: string;
	}

	export interface Instance {
		/**
		 * Generates all the required noises for given coordinates.
		 */
		coord(xz: XZ): Noise;
		/**
		 * Because we're using Rust (probably), we need to free the memory.
		 */
		free(): void;
	}
}

/**
 * Define individual noises to make up a biome.
 */
export type NoiseSource = (props: NoiseSource.Props) => NoiseSource.Instance;
